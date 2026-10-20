
#ifndef EAS_VISIBILITY_H
#define EAS_VISIBILITY_H

#ifdef SONIVOX_STATIC_DEFINE
#  define EAS_PUBLIC
#  define EAS_PRIVATE
#else
#  ifndef EAS_PUBLIC
#    ifdef sonivox_EXPORTS
        /* We are building this library */
#      define EAS_PUBLIC 
#    else
        /* We are using this library */
#      define EAS_PUBLIC 
#    endif
#  endif

#  ifndef EAS_PRIVATE
#    define EAS_PRIVATE 
#  endif
#endif

#ifndef SONIVOX_DEPRECATED
#  define SONIVOX_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef SONIVOX_DEPRECATED_EXPORT
#  define SONIVOX_DEPRECATED_EXPORT EAS_PUBLIC SONIVOX_DEPRECATED
#endif

#ifndef SONIVOX_DEPRECATED_NO_EXPORT
#  define SONIVOX_DEPRECATED_NO_EXPORT EAS_PRIVATE SONIVOX_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef SONIVOX_NO_DEPRECATED
#    define SONIVOX_NO_DEPRECATED
#  endif
#endif

#endif /* EAS_VISIBILITY_H */
