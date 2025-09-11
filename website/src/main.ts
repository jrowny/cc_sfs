// Fetch version from manifest.json and update the version display
async function updateVersionFromManifest() {
  try {
    const response = await fetch("./manifest.json");
    if (!response.ok) {
      console.warn("Could not fetch manifest.json, using default version");
      return;
    }

    const manifest = await response.json();
    const version = manifest.version;

    if (version) {
      // Find the version element and update it
      const versionElement = document.getElementById("version-display");
      if (versionElement) {
        versionElement.textContent = `Version: ${version}`;
      }
    }
  } catch (error) {
    console.warn("Error fetching version from manifest:", error);
  }
}

// Wait for DOM to be ready, then update version
document.addEventListener("DOMContentLoaded", updateVersionFromManifest);
