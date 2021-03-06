{
	"targets": [{
		"target_name": "ui",
		"include_dirs": ["<(module_root_dir)/src/includes", "<(module_root_dir)"],
		"sources": [
			'<!@(node tools/list-sources.js)'
		],
		"conditions": [
			["OS=='win'", {
				"libraries": [
					"<(module_root_dir)/libui.lib"
				]
			}],
			["OS=='linux'", {
				"cflags": ["-fvisibility=hidden" ,"-std=gnu11"],
				'ldflags': [
					"-Wl,-rpath,'$$ORIGIN',-rpath,<(module_root_dir)",
				],
				"libraries": [
					"<(module_root_dir)/libui.so"
				],
				'include_dirs': [
					'<!@(pkg-config gtk+-3.0 --cflags-only-I | sed s/-I//g)'
				]
			}],
			["OS=='mac'", {
				"xcode_settings": {
					"OTHER_LDFLAGS": [
						"-fvisibility=hidden",
						"-L<(module_root_dir)",
						"-lui",
						"-rpath",
						"'@loader_path'",
						"-rpath",
						"<(module_root_dir)"
					]
				}
			}],
		]
	}, {
		"target_name": "copy_binary",
		"type":"none",
		"dependencies": [ "ui" ],
		"copies": [
			{
				'destination': '<(module_root_dir)',
				'files': ['<(module_root_dir)/build/Release/ui.node']
			}
		]
	}]
}
