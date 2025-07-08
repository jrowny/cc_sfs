import Settings from './Settings'
import Status from './Status'
import Logs from './Logs'
import Update from './Update'
import About from './About'

function App() {

  return (
    <div class="flex flex-col items-center h-screen pt-10 bg-base-200 ">
      <h1 class="text-xl font-bold w-full max-w-5xl pl-1 pb-4">Elegoo Centauri Carbon <span class="text-accent">X</span> BigTreeTech SFS 2.0</h1>
      <div class="tabs tabs-lift w-full max-w-5xl">

        <input type="radio" name="tabs" class="tab" aria-label="Status" checked />
        <div class="tab-content bg-base-100 border-base-300 p-6">
          <Status />
        </div>

        <input type="radio" name="tabs" class="tab" aria-label="Settings" />
        <div class="tab-content bg-base-100 border-base-300 p-6">
          <Settings />
        </div>

        <input type="radio" name="tabs" class="tab" aria-label="Logs" />
        <div class="tab-content bg-base-100 border-base-300 p-6">
          <Logs />
        </div>

        <input type="radio" name="tabs" class="tab" aria-label="Update" />
        <div class="tab-content bg-base-100 border-base-300 p-6">
          <Update />
        </div>
        <input type="radio" name="tabs" class="tab" aria-label="About" />
        <div class="tab-content bg-base-100 border-base-300 p-6">
          <About />
        </div>

      </div>
    </div>
  )
}

export default App
