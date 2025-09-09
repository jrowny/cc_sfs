import { A, useLocation } from '@solidjs/router'
import { ParentProps } from 'solid-js'

function App(props: ParentProps) {
  const location = useLocation()

  const isActive = (path: string) => {
    const pathname = location.pathname
    if (path === '/' || path === '/status') {
      return pathname === '/' || pathname === '/status'
    }
    return pathname === path
  }

  return (
    <div class="flex flex-col items-center h-screen pt-10 bg-base-200 ">
      <h1 class="text-xl font-bold w-full max-w-5xl pl-1 pb-4">Elegoo Centauri Carbon <span class="text-accent">X</span> BigTreeTech SFS 2.0</h1>
      <div class="tabs tabs-lift w-full max-w-5xl">

        <A href="/status" class={`tab ${isActive('/status') ? 'tab-active' : ''}`}>
          Status
        </A>

        <A href="/settings" class={`tab ${isActive('/settings') ? 'tab-active' : ''}`}>
          Settings
        </A>

        <A href="/logs" class={`tab ${isActive('/logs') ? 'tab-active' : ''}`}>
          Logs
        </A>

        <A href="/update" class={`tab ${isActive('/update') ? 'tab-active' : ''}`}>
          Update
        </A>

        <A href="/about" class={`tab ${isActive('/about') ? 'tab-active' : ''}`}>
          About
        </A>

      </div>

      <div class="w-full max-w-5xl bg-base-100 border-base-300 p-6">
        {props.children}
      </div>
    </div>
  )
}

export default App
