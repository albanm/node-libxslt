{
  "targets": [
    {
      "target_name": "node-libxslt",
      "sources": [ "src/node_libxslt.cc", "src/stylesheet.cc" ],
      "include_dirs": ["<!(node -e \"require('nan')\")"],
      'dependencies': [
      	'./deps/libxslt.gyp:libxslt',
      	'./deps/libxslt.gyp:libexslt',
        './node_modules/libxmljs/vendor/libxml/libxml.gyp:libxml',
        './node_modules/libxmljs/binding.gyp:xmljs'
      ]
    }
  ]
}