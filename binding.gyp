{
  "targets": [
    {
      "target_name": "node-libxslt",
      "sources": [
        "src/node_libxslt.cc",
        "src/stylesheet.cc",
        "src/extensions.cc"
      ],
      "cflags" : [ "-O0 -g" ],
      "include_dirs": ["<!(node -e \"require('nan')\")"],
      'dependencies': [
      	'./deps/libxslt.gyp:libxslt',
      	'./deps/libxslt.gyp:libexslt'
      ]
    }
  ]
}
