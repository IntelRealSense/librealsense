{
  'variables': {
  },
  "targets": [
    {
      "target_name": "node_librealsense",
      "variables": {
      },
      "sources": [
        "./src/addon.cpp",
      ],
      "include_dirs": [
         "./librealsense/include",
         "<!(node -e \"require('nan')\")",
      ],
      "cflags!": [
        "-fno-exceptions"
      ],
      "cflags": [
        "-Wno-deprecated-declarations",
        "-Wno-switch",
        "-std=c++11",
        "-fstack-protector-strong",
        "-fPIE -fPIC",
        "-O2 -D_FORTIFY_SOURCE=2",
        "-Wformat -Wformat-security"
      ],
      "cflags_cc!": [
        "-fno-exceptions"
      ],
      "cflags_cc": [
        "-Wno-switch",
        "-Wno-deprecated-declarations"
      ],
      "conditions": [
        [
          "OS==\"win\"",
          {
            'msvs_settings': {
              'VCCLCompilerTool': {
                'WarnAsError': 'false',
                'DisableSpecificWarnings': ['4996', '4244'],
                'SuppressStartupBanner': 'true',
              }
            },
            "libraries": [
              "<(module_root_dir)/librealsense/build/Release/realsense2.lib",
            ],
          }
        ],
        ['OS=="mac"',
          {
            'xcode_settings': {
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
            }
          }
        ],
        [
          "OS!=\"win\"",
          {
            "libraries": [
              "<(module_root_dir)/librealsense/build/librealsense2.so",
            ],
            'ldflags': [
              '-Wl,-rpath,\$$ORIGIN/../../librealsense/build',
            ],
            "cflags+": [
              "-std=c++11"
            ],
            "cflags_c+": [
              "-std=c++14"
            ],
            "cflags_cc+": [
              "-std=c++14"
            ]
          }
        ]
      ]
    },
    {
      "target_name": "copy_dll",
      "type":"none",
      "dependencies" : [ "node_librealsense" ],
      "conditions": [
        ['OS=="win"', {
           "copies":
            [
              {
                'destination': '<(module_root_dir)/build/Release',
                'files': ['<(module_root_dir)/librealsense/build/Release/realsense2.dll']
              }
            ]
        }]
      ]
    }
  ]
}
