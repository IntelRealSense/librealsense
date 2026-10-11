import type { ControlGroup, OptionInfo } from '../api/types'
import { optionLabel } from '../api/types'

/**
 * The options a search leaves a group: the ones whose label contains the typed text, all of
 * them when the group's own name is what matched, or null when the search excludes the group
 * entirely. Descriptions are not searched - they mention unrelated terms (the sync-mode
 * description contains "laser") and produced false hits.
 *
 * An empty query returns every option, in the original order.
 */
export function searchGroup(group: ControlGroup, query: string): OptionInfo[] | null {
  const q = query.trim().toLowerCase()
  if (!q) return group.options

  const matching = group.options.filter(o => optionLabel(o.option_id).toLowerCase().includes(q))
  if (matching.length > 0) return matching
  return optionLabel(group.name).toLowerCase().includes(q) ? group.options : null
}
