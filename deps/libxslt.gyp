{
  'variables': { 'target_arch%': 'x64' },
  'target_defaults': {
    'defines': ['HAVE_CONFIG_H','LIBXSLT_STATIC'],
    'cflags': [
              '-g',
            ]
  },
  'targets': [
    {
      'target_name': 'libxslt',
      'type': 'static_library',
      'sources': [
        'libxslt/libxslt/attributes.c',
        'libxslt/libxslt/attrvt.c',
        'libxslt/libxslt/documents.c',
        'libxslt/libxslt/extensions.c',
        'libxslt/libxslt/extra.c',
        'libxslt/libxslt/functions.c',
        'libxslt/libxslt/imports.c',
        'libxslt/libxslt/keys.c',
        'libxslt/libxslt/namespaces.c',
        'libxslt/libxslt/numbers.c',
        'libxslt/libxslt/pattern.c',
        'libxslt/libxslt/preproc.c',
        'libxslt/libxslt/preproc.h',
        'libxslt/libxslt/security.c',
        'libxslt/libxslt/security.h',
        'libxslt/libxslt/templates.c',
        'libxslt/libxslt/transform.c',
        'libxslt/libxslt/variables.c',
        'libxslt/libxslt/xslt.c',
        'libxslt/libxslt/xsltlocale.c',
        'libxslt/libxslt/xsltutils.c'
      ],
      'include_dirs': [
        # platform and arch-specific headers
        'libxslt/config/<(OS)/<(target_arch)'
      ],
      'dependencies': [
        '../node_modules/libxmljs/vendor/libxml/libxml.gyp:libxml',
      ],
      'direct_dependent_settings': {
        'defines': ['LIBXSLT_STATIC'],
        'include_dirs': ['libxslt']
      }
    },
    {
      'target_name': 'libexslt',
      'type': 'static_library',
      'sources': [
        'libxslt/libexslt/common.c',
        'libxslt/libexslt/crypto.c',
        'libxslt/libexslt/date.c',
        'libxslt/libexslt/dynamic.c',
        'libxslt/libexslt/exslt.c',
        'libxslt/libexslt/functions.c',
        'libxslt/libexslt/math.c',
        'libxslt/libexslt/saxon.c',
        'libxslt/libexslt/sets.c',
        'libxslt/libexslt/strings.c'
      ],
      'include_dirs': [
        # platform and arch-specific headers
        'libxslt/config/<(OS)/<(target_arch)'
      ],
      'dependencies': [
        '../node_modules/libxmljs/vendor/libxml/libxml.gyp:libxml',
        'libxslt'
      ],
      'direct_dependent_settings': {
        'include_dirs': ['libexslt']
      }
    }
  ]
}