function hideElement(element)
{
  function doHide()
  {
    let propertyName = "display";
    let propertyValue = "none";
    if (element.localName == "frame")
    {
      propertyName = "visibility";
      propertyValue = "hidden";
    }

    if (element.style.getPropertyValue(propertyName) != propertyValue ||
        element.style.getPropertyPriority(propertyName) != "important")
      element.style.setProperty(propertyName, propertyValue, "important");
  }

  doHide();

  new MutationObserver(doHide).observe(
    element, {
      attributes: true,
      attributeFilter: ["style"]
    }
  );
}
