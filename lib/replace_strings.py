import fileinput
import os

"""files_to_search = ['../src/chrome/app/chromium_strings.grd', '../src/chrome/app/settings_chromium_strings.grdp',
'../src/chrome/browser/ui/android/strings/android_chrome_strings.grd', '../src/components/components_chromium_strings.grd',
'../src/components/components_strings.grd', '../src/chrome/app/generated_resources.grd']"""

files_to_search = []

browser_name = "Carbon"
company_name = "Sutherland Enterprises Ltd"

words_to_replace_1 = ["Chromiumista", "Chromiumist", "Chromiumilla", "Chromiumille", "Chromiumile", "Chromiumil", "Chromiumiin", "Chromiumin",
"Chromiumia", "Chromiumi", "Chromiuma", "Chromiumu", "Chromiumova", "Chromiumove", "Chromiumov", "Chromiumom", "Chromiumot",
"Chromiumnak", "Chromiumban", "Chromiumba", "Chromiumhoz", "Chromiumra", "Chromiummal", "Chromiumos", "Chromiumon", "Chromiumja", "Chromiums", "Chromium", "Freedom Browser"]

# What needs to be fixed
fixer_wrong_1 = ["Carbon <ph name=\"VERSION_CHROMIUM\">", "Carbon Authors", "Carbon OS"]
# Used to fix..
fixer_correct_1 = ["Chromium <ph name=\"VERSION_CHROMIUM\">", "Chromium Authors", "Chromium OS"]

words_to_replace_2 = ["Google Chrome", "Google Chromu", "Google Chroma", "Google Chromom", "Google Chromovimi", "Google Chromovim", "Google Chromovi",
"Google Chromove", "Google Chromovo", "Google Chromova", "Google Chromov"]

# Carbon OS -> Google Chrome OS

words_to_replace_3 = ["Chrome", "Chromuim", "Chromu", "Chroma", "Chromom", "Chromovimi", "Chromovim", "Chromovi", "Chromovem", "Chromovega", "Chromove",
"Chromovo", "Chromova", "Chromov"]

# The Carbon Authors -> Sutherland Enterprises Ltd

# Carbon OS -> Chrome OS

# Google Inc -> Sutherland Enterprises Ltd


# -----= Start =-----
# Search for files to translate
directories_to_search = ['../src/chrome/browser/ui/android/strings', '../src/components', '../src/chrome/app', '../src/chrome/app/resources']
for directory in directories_to_search:
	for root, dirs, files in os.walk(directory):
		for file in files:
			if file.endswith(".grd") or file.endswith(".xtb") or file.endswith("grdp"):
				print("Adding " + root + file + " to file search list")
				files_to_search.append(os.path.join(root, file))

# Replace the strings in the files specified from the search above
for i in range(0, len(files_to_search)):
	with fileinput.FileInput(files_to_search[i], inplace=True) as file:
		for line in file:
			counter = 0

			# Loop through the first list of words
			while counter != len(words_to_replace_1):
				if words_to_replace_1[counter] in line:
					line = line.replace(words_to_replace_1[counter], browser_name)
				counter += 1

			# Check for any corrections

			# Reset counter
			counter = 0

			while counter != len(fixer_wrong_1):
				if words_to_replace_1[counter] in line:
					line = line.replace(fixer_wrong_1[counter], fixer_correct_1[counter])
				counter += 1

			# Reset counter
			counter = 0

			# Next set of words to check
			while counter != len(words_to_replace_2):
				if words_to_replace_2[counter] in line:
					line = line.replace(words_to_replace_2[counter], browser_name)
				counter += 1

			# Check for Carbon OS
			if "Carbon OS" in line:
				line = line.replace("Carbon OS", "Google Chrome OS")

			# Reset counter
			counter = 0

			# Next set of words to check
			while counter != len(words_to_replace_3):
				if words_to_replace_3[counter] in line:
					line = line.replace(words_to_replace_3[counter], browser_name)
				counter += 1

			if "Super Fast Browser" in line:
				line = line.replace("Super Fast Browser", "Carbon")

			# Check for The Carbon Authors
			if "The Carbon Authors" in line:
				line = line.replace("The Carbon Authors", "Sutherland Enterprises Ltd")
			if "Carbon Authors" in line:
				line = line.replace("Carbon Authors", "Sutherland Enterprises Ltd")

			# Check for Carbon OS
			if "Carbon OS" in line:
				line = line.replace("Carbon OS", "Chrome OS")

			# Check for Carbon OS
			if "Google Inc" in line:
				line = line.replace("Google Inc", company_name)

			if "Google LLC" in line:
				line = line.replace("Google LLC", company_name)

			# fix showing "google Carbon storage" in app storage management in settings
			if 'Google <ph name="APP_NAME">%1$s<ex>Carbon</ex></ph> storage' in line:
				line = line.replace('Google <ph name="APP_NAME">%1$s<ex>Carbon</ex></ph> storage', '<ph name="APP_NAME">%1$s<ex>Carbon</ex></ph> storage')
			if 'Google <ph name="APP_NAME" /> storage' in line:
				line = line.replace('Google <ph name="APP_NAME" /> storage', '<ph name="APP_NAME" /> storage')


			#----- Replace incognito with private -----#
			if "You're incognito" in line:
				line = line.replace("You're incognito", "This is a private tab")

			if "an incognito" in line:
				line = line.replace("an incognito", "a private")

			if "an Incognito" in line:
				line = line.replace("an Incognito", "a private")

			if "incognito" in line:
				line = line.replace("incognito", "private")

			if "Incognito" in line:
				line = line.replace("Incognito", "private")

			if "Carbon" in line:
				line = line.replace("Carbon", "Carbon Browser")

			if "Sutherland Enterprises Ltd" in line:
				line = line.replace("Sutherland Enterprises Ltd", "Carbon X Labs")

			# Print the line to the document
			print(line, end='')

print("Done.")
