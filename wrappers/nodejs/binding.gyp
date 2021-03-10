{
  'variables': {
    'configuration%': '${BUILDTYPE}',
    'build_arch': '<!(node -p "process.arch")',
    'vs_configuration%': "Debug",
    'win_realsense_dir': '<(module_root_dir)/../../build',
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
         "../../include",
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
            "conditions": [
            	[ 'vs_configuration=="Debug"', {
            		"libraries": [ "<(win_realsense_dir)/Debug/realsense2d.lib" ]
            	}
            	],
            	[ 'vs_configuration=="Release"', {
            		"libraries": [ "<(win_realsense_dir)/Release/realsense2.lib" ]
            	}
            	],
            ]
          }
        ],
       ['OS=="mac"',
          {
            "libraries": [
              '<(module_root_dir)/../../build/<(configuration)/librealsense2.dylib',
              # Write the below RPATH into the generated addon
              '-Wl,-rpath,@loader_path/../../../../build/<(configuration)',
            ],
            'xcode_settings': {
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
              'CLANG_CXX_LIBRARY': 'libc++',
              'MACOS_DEPLOYMENT_TARGET': '10.12',
              'CLANG_CXX_LANGUAGE_STANDARD': 'c++14'
            }
          }
        ],
        ['OS=="linux"',
          {
            "libraries": [
              "-lrealsense2"
            ],
            'ldflags': [
              # rpath for build from source
              '-Wl,-rpath,\$$ORIGIN/../../../../build',
              '-L<(module_root_dir)/../../build',
              # rpatch for build debian package
              '-Wl,-rpath,\$$ORIGIN/../../../../obj-x86_64-linux-gnu',
              '-L<(module_root_dir)/../../obj-x86_64-linux-gnu'
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
                "conditions": [
            	[ 'vs_configuration=="Debug"', {
            		'files': ['<(win_realsense_dir)/Debug/realsense2d.dll']
            	}
            	],
            	[ 'vs_configuration=="Release"', {
            		'files': ['<(win_realsense_dir)/Release/realsense2.dll']
            	}
            	],
            ]
              }
            ]
        }]
      ]
    }
  ]
}
