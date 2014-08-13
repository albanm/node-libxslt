{
  'variables': {
    'target_arch%': 'ia32', # build for a 32-bit CPU by default
    'xmljs_include_dirs%': [],
    'xmljs_libraries%': [],
  },
  'target_defaults': {
    'default_configuration': 'Debug',
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 1, # static debug
          },
        },
      },
      'Release': {
        'defines': [ 'NDEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 0, # static release
          },
        },
      }
    },
    'defines': ['HAVE_CONFIG_H','LIBXSLT_STATIC'],
  },
  'targets': [
    {
      'target_name': 'libxslt',
      'type': 'static_library',
      'sources': [
        'libxslt/attributes.c',
        'libxslt/attrvt.c',
        'libxslt/documents.c',
        'libxslt/extensions.c',
        'libxslt/extra.c',
        'libxslt/functions.c',
        'libxslt/imports.c',
        'libxslt/keys.c',
        'libxslt/namespaces.c',
        'libxslt/numbers.c',
        'libxslt/pattern.c',
        'libxslt/preproc.c',
        'libxslt/preproc.h',
        'libxslt/security.c',
        'libxslt/security.h',
        'libxslt/templates.c',
        'libxslt/transform.c',
        'libxslt/variables.c',
        'libxslt/xslt.c',
        'libxslt/xsltlocale.c',
        'libxslt/xsltutils.c'
      ],
      'include_dirs': [
        # platform and arch-specific headers
        'config/<(OS)/<(target_arch)',
        '<@(xmljs_include_dirs)'
      ],
      'link_settings': {
	      'libraries': [
	        '<@(xmljs_libraries)',
	      ]
	    },
      'direct_dependent_settings': {
        'defines': ['LIBXSLT_STATIC'],
        'include_dirs': ['libxslt', '<@(xmljs_include_dirs)'],
      }
    },
    {
      'target_name': 'libexslt',
      'type': 'static_library',
      'sources': [
        'libexslt/common.c',
        'libexslt/crypto.c',
        'libexslt/date.c',
        'libexslt/dynamic.c',
        'libexslt/exslt.c',
        'libexslt/functions.c',
        'libexslt/math.c',
        'libexslt/saxon.c',
        'libexslt/sets.c',
        'libexslt/strings.c'
      ],
      'include_dirs': [
        # platform and arch-specific headers
        'config/<(OS)/<(target_arch)',
        '<@(xmljs_include_dirs)',
        '.'
      ],
      'dependencies': [
        'libxslt'
      ],
      'link_settings': {
	      'libraries': [
	        '<@(xmljs_libraries)'
	      ]
	    },
      'direct_dependent_settings': {
        'include_dirs': ['libexslt']
      }
    }
  ]
}