import { useState } from 'react'
import type { ReactNode } from 'react'

/** How the three levels of the controls tree are drawn: sensor, section, group. */
const VARIANTS = {
  sensor: {
    className: 'bg-gray-800/50 rounded-lg px-2 py-1',
    headerClassName: 'flex items-center justify-between',
    toggleClassName: 'flex items-center gap-2 flex-1 min-w-0 text-left',
    chevronClassName: 'w-3 h-3',
  },
  section: {
    className: 'border border-gray-700 rounded overflow-hidden',
    headerClassName: 'flex items-center bg-gray-750 hover:bg-gray-700 transition-colors',
    toggleClassName: 'flex-1 flex items-center gap-1.5 p-1.5 min-w-0 text-left',
    chevronClassName: 'w-3 h-3',
  },
  group: {
    className: 'border border-gray-600 rounded overflow-hidden',
    headerClassName: 'flex items-center justify-between p-1.5 bg-gray-700/50 hover:bg-gray-700 transition-colors',
    toggleClassName: 'flex items-center gap-1.5 flex-1 min-w-0 text-left',
    chevronClassName: 'w-2.5 h-2.5',
  },
} as const

interface CollapsibleProps {
  variant: keyof typeof VARIANTS
  /** Rendered inside the toggle button, after the chevron. */
  label: ReactNode
  /** Rendered next to the toggle, outside it, so it stays independently clickable. */
  aside?: ReactNode
  /** Rendered under the header whether open or closed (errors, status lines). */
  belowHeader?: ReactNode
  /** Pins it open and ignores clicks, so the user's own choice survives a search. */
  forcedOpen?: boolean
  children: ReactNode
}

export function Collapsible({ variant, label, aside, belowHeader, forcedOpen, children }: CollapsibleProps) {
  const style = VARIANTS[variant]
  const [localOpen, setLocalOpen] = useState(false)
  const isOpen = forcedOpen || localOpen
  const toggle = () => {
    if (!forcedOpen) setLocalOpen(o => !o)
  }

  return (
    <div className={style.className}>
      <div className={style.headerClassName}>
        <button onClick={toggle} aria-expanded={isOpen} className={style.toggleClassName}>
          <CollapseChevron isOpen={isOpen} className={style.chevronClassName} />
          {label}
        </button>
        {aside}
      </div>
      {belowHeader}
      {isOpen && children}
    </div>
  )
}

/** On/off pill switch, sized to sit in a collapsible header as its `aside`. */
export function ToggleSwitch({ enabled, onToggle }: { enabled: boolean; onToggle: () => void }) {
  return (
    <button
      onClick={onToggle}
      aria-pressed={enabled}
      className={`relative w-8 h-4 rounded-full transition-colors ${enabled ? 'bg-rs-blue' : 'bg-gray-600'}`}
    >
      <span
        className={`absolute top-0.5 left-0.5 w-3 h-3 rounded-full bg-white transition-transform ${
          enabled ? 'translate-x-4' : ''
        }`}
      />
    </button>
  )
}

/** Right-pointing chevron that rotates down when open. */
function CollapseChevron({ isOpen, className }: { isOpen: boolean; className: string }) {
  return (
    <svg
      className={`${className} shrink-0 text-gray-400 transition-transform ${isOpen ? 'rotate-90' : ''}`}
      fill="none" stroke="currentColor" viewBox="0 0 24 24"
    >
      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
    </svg>
  )
}
