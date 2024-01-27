package org.chromium.chrome.browser.vdl;

import android.app.Application;
import org.chromium.chrome.browser.vdl.crypto.Crypto;
import android.os.Environment;
import android.util.Log;
import android.arch.lifecycle.ViewModel;
import android.arch.lifecycle.MutableLiveData;
import android.arch.lifecycle.LiveData;
import org.chromium.chrome.browser.vdl.utils.HLSUtils;

import android.util.Pair;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.*;

public class VDLViewModel extends ViewModel {

    private static final String BANDWIDTH = "BANDWIDTH";
    private static final String EXT_X_KEY = "#EXT-X-KEY";
    private static final String TAG = "VDLViewModel";

    private final MutableLiveData<String> _progressLiveData = new MutableLiveData<>();
    public final LiveData<String> progress = _progressLiveData;

    public VDLViewModel(Application app) {
        //super(app);
    }

    public void downloadFile(String address) {
        try {
            URL url = new URL(address);
            String outputFileAddress = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getPath() + "/outputHLS" + new Random().nextInt() + ".mp4";
            File outputFile = new File(outputFileAddress);

            _progressLiveData.postValue("Getting master play list...");
            Pair<URL, List<String>> maximumResolutionPlayList = getMasterPlaylist(url, url);
            Crypto crypto = new Crypto(HLSUtils.getBaseUrl(url), null);

            boolean isPointingToSameFile = false;
            String urlPointingToSingleResource = "";

            for (String item : maximumResolutionPlayList.second) {
                if (item.contains("#EXT-X-BYTE-RANGE")) {
                    isPointingToSameFile = true;
                }

                if (isPointingToSameFile) {
                    if (!item.startsWith("#")) {
                        urlPointingToSingleResource = item.startsWith("http") ? item : HLSUtils.getBaseUrl(maximumResolutionPlayList.first != null ? maximumResolutionPlayList.first : url) + item;
                    }

                    break;
                }
            }

            if (isPointingToSameFile) {
                try {
                    HttpURLConnection connection = (HttpURLConnection) new URL(urlPointingToSingleResource).openConnection();
                    connection.connect();

                    if (connection.getResponseCode() == HttpURLConnection.HTTP_OK) {
                        int fileLength = connection.getContentLength();
                        FileOutputStream outputStream = new FileOutputStream(outputFile);
                        InputStream inputStream = connection.getInputStream();
                        byte[] buffer = new byte[4096];
                        long totalBytesRead = 0;

                        int bytesRead;
                        while ((bytesRead = inputStream.read(buffer)) != -1) {
                            outputStream.write(buffer, 0, bytesRead);
                            totalBytesRead += bytesRead;
                            long progress = (totalBytesRead * 100L) / fileLength;
                            _progressLiveData.postValue(String.valueOf(progress));
                        }

                        inputStream.close();
                        outputStream.close();
                        connection.disconnect();
                    }

                } catch (Exception e) {
                    e.printStackTrace();
                }
            } else {
                for (int index = 0; index < maximumResolutionPlayList.second.size(); index++) {
                    String line = maximumResolutionPlayList.second.get(index).trim();
                    if (line.startsWith(EXT_X_KEY)) {
                        crypto.updateKeyString(line);
                    } else if (!line.isEmpty() && !line.startsWith("#")) {
                        URL segmentUrl = !line.startsWith("http") ? new URL(HLSUtils.getBaseUrl(maximumResolutionPlayList.first != null ? maximumResolutionPlayList.first : url) + line) : new URL(line);
                        downloadFile(segmentUrl, outputFile, crypto);
                        long progress = index * 100 / maximumResolutionPlayList.second.size();
                        _progressLiveData.postValue(String.valueOf(progress));
                    }
                }
            }
        } catch (Exception e) {}
    }

    private void downloadFile(URL url, File outputFile, Crypto crypto) {
        byte[] buffer = new byte[512];
        try {
            InputStream inputStream = crypto.hasKey() ? crypto.wrapInputStream(url.openStream()) : url.openStream();
            if (!outputFile.exists()) {
                outputFile.createNewFile();
            }

            FileOutputStream outputStream = new FileOutputStream(outputFile, true);

            int read;
            while ((read = inputStream.read(buffer)) >= 0) {
                outputStream.write(buffer, 0, read);
            }
            inputStream.close();
            outputStream.close();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private Pair<URL, List<String>> getMasterPlaylist(URL mainUrl, URL subUrl) {
        if (subUrl == null) {
            throw new RuntimeException("Sub URL cannot be null");
        }

        boolean isMaster = false;
        long maxRate = 0L;
        int maxRateIndex = 0;

        List<String> playlist = new ArrayList<>();

        try {
            BufferedReader reader = new BufferedReader(new InputStreamReader(subUrl.openStream()));
            String line;
            int index = 0;

            while ((line = reader.readLine()) != null) {
                playlist.add(line);

                if (line.contains(BANDWIDTH)) {
                    isMaster = true;
                }

                if (isMaster && line.contains(BANDWIDTH)) {
                    try {
                        int pos = line.lastIndexOf("$BANDWIDTH=") + 10;
                        int end = line.indexOf(",", pos);
                        if (end < 0 || end < pos) {
                            end = line.length() - 1;
                        }
                        long bandwidth = Long.parseLong(line.substring(pos, end));
                        maxRate = Math.max(bandwidth, maxRate);

                        if (bandwidth == maxRate) {
                            maxRateIndex = index + 1;
                        }

                    } catch (NumberFormatException e) {
                        e.printStackTrace();
                    }
                }

                index++;
            }

            reader.close();

        } catch (Exception e) {
            _progressLiveData.postValue("ERROR: " + e.getMessage());
            e.printStackTrace();
        }

        if (isMaster) {
            URL newMainUrl = HLSUtils.updateUrlForSubPlaylist(playlist.get(maxRateIndex), mainUrl.toString());
            return getMasterPlaylist(newMainUrl, newMainUrl);
        } else {
            return new Pair<>(mainUrl, playlist);
        }
    }
}
