{
  "targets": [
    {
      "target_name": "node-libxslt",
      "sources": [
        "src/node_libxslt.cc",
        "src/stylesheet.cc",
        "src/extensions.cc"
      ],
      "cflags" : [ "-std=c++11" ],
      "include_dirs": ["<!(node -e \"require('nan')\")"],
      'dependencies': [
      	'./deps/libxslt/libxslt.gyp:libxslt',
      	'./deps/libxslt/libxslt.gyp:libexslt'
      ]
    }
  ]
}
