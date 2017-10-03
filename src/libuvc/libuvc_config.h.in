#ifndef LIBUVC_CONFIG_H
#define LIBUVC_CONFIG_H

#define LIBUVC_VERSION_MAJOR @libuvc_VERSION_MAJOR@
#define LIBUVC_VERSION_MINOR @libuvc_VERSION_MINOR@
#define LIBUVC_VERSION_PATCH @libuvc_VERSION_PATCH@
#define LIBUVC_VERSION_STR "@libuvc_VERSION@"
#define LIBUVC_VERSION_INT                      \
  ((@libuvc_VERSION_MAJOR@ << 16) |             \
   (@libuvc_VERSION_MINOR@ << 8) |              \
   (@libuvc_VERSION_PATCH@))

/** @brief Test whether libuvc is new enough
 * This macro evaluates true iff the current version is
 * at least as new as the version specified.
 */
#define LIBUVC_VERSION_GTE(major, minor, patch)                         \
  (LIBUVC_VERSION_INT >= (((major) << 16) | ((minor) << 8) | (patch)))

#cmakedefine LIBUVC_HAS_JPEG 1

#endif // !def(LIBUVC_CONFIG_H)
