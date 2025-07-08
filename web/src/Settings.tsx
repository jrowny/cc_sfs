import { createSignal, onMount } from 'solid-js'

function Settings() {
  const [ssid, setSsid] = createSignal('')
  const [password, setPassword] = createSignal('')
  const [elegooip, setElegooip] = createSignal('')
  const [timeout, setTimeoutValue] = createSignal(2000)
  const [loading, setLoading] = createSignal(true)
  const [error, setError] = createSignal('')
  const [saveSuccess, setSaveSuccess] = createSignal(false)
  const [apMode, setApMode] = createSignal<boolean | null>(null);
  // Load settings from the server and scan for WiFi networks
  onMount(async () => {
    try {
      setLoading(true)

      // Load settings
      const response = await fetch('/get_settings')
      if (!response.ok) {
        throw new Error(`Failed to load settings: ${response.status} ${response.statusText}`)
      }
      const settings = await response.json()

      setSsid(settings.ssid || '')
      // Password won't be loaded from server for security
      setPassword('')
      setElegooip(settings.elegooip || '')
      setTimeoutValue(settings.timeout || 2000)
      setApMode(settings.ap_mode || null)

      setError('')
    } catch (err: any) {
      setError(`Error loading settings: ${err.message || 'Unknown error'}`)
      console.error('Failed to load settings:', err)
    } finally {
      setLoading(false)
    }
  })


  const handleSave = async () => {
    try {
      setSaveSuccess(false)
      setError('')

      const settings = {
        ssid: ssid(),
        passwd: password(),
        ap_mode: false,
        elegooip: elegooip(),
        timeout: timeout(),
      }

      const response = await fetch('/update_settings', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(settings)
      })

      if (!response.ok) {
        throw new Error(`Failed to save settings: ${response.status} ${response.statusText}`)
      }

      setSaveSuccess(true)
      setTimeout(() => setSaveSuccess(false), 3000)
    } catch (err: any) {
      setError(`Error saving settings: ${err.message || 'Unknown error'}`)
      console.error('Failed to save settings:', err)
    }
  }

  return (
    <div class="card" >


      {loading() ? (
        <p>Loading settings.. <span class="loading loading-spinner loading-xl"></span>.</p>
      ) : (
        <div>
          {error() && (
            <div role="alert" class="mb-4 alert alert-error">
              {error()}
            </div>
          )}

          {saveSuccess() && (
            <div role="alert" class="mb-4 alert alert-success">
              Settings saved successfully!
            </div>
          )}

          <h2 class="text-lg font-bold mb-4">Wifi Settings</h2>
          {apMode() && (
            <div>
              <fieldset class="fieldset ">
                <legend class="fieldset-legend">SSID</legend>
                <input
                  type="text"
                  id="ssid"
                  value={ssid()}
                  onInput={(e) => setSsid(e.target.value)}
                  placeholder="Enter WiFi network name..."
                  class="input"
                />
              </fieldset>


              <fieldset class="fieldset">
                <legend class="fieldset-legend">Password</legend>
                <input
                  type="password"
                  id="password"
                  value={password()}
                  onInput={(e) => setPassword(e.target.value)}
                  placeholder="Enter WiFi password..."
                  class="input"
                />
              </fieldset>


              <div role="alert" class="mt-4 alert alert-info alert-soft">
                <span>Note: after changing the wifi network you may need to enter a new IP address to get to this device. If the wifi connection fails, the device will revert to AP mode and you can reconnect by connecting to the Wifi network named ElegooXBTTSFS20. If your network supports MDNS discovery you can also find this device at <a class="link link-accent" href="http://ccxsfs20.local">
                  ccxsfs20.local</a></span>
              </div>
            </div>
          )
          }
          {
            !apMode() && (
              <button class="btn" onClick={() => setApMode(true)}>Change Wifi network</button>
            )
          }

          <h2 class="text-lg font-bold mb-4 mt-10">Device Settings</h2>


          <fieldset class="fieldset">
            <legend class="fieldset-legend">Elegoo Centauri Carbon IP Address</legend>
            <input
              type="text"
              id="elegooip"
              value={elegooip()}
              onInput={(e) => setElegooip(e.target.value)}
              placeholder="xxx.xxx.xxx.xxx"
              class="input"
            />
          </fieldset>


          <fieldset class="fieldset">
            <legend class="fieldset-legend">Movment Sensor Timeout</legend>
            <input
              type="number"
              id="timeout"
              value={timeout()}
              onInput={(e) => setTimeoutValue(parseInt(e.target.value) || 5000)}
              min="100"
              max="30000"
              step="100"
              class="input"
            />
            <p class="label">Value in milliseconds between reading from the movement sensor</p>
          </fieldset>

          <button
            class="btn btn-accent btn-soft mt-10"
            onClick={handleSave}
          >
            Save Settings
          </button>
        </div >
      )
      }
    </div >
  )
}

export default Settings 