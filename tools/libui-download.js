const fs = require('fs');
const os = require('os');
const path = require('path');
const mkdirp = require('mkdirp');
const _debug = require('debug');
const tar = require('tar');
const {mkCacheDir, cacheDir, buildUrl, requestHttps, doDownload} =
	require('./lib')('libui');

const debug = _debug('libui-download');

function nodePlatformToOS(arch) {

	switch (arch) {
		case 'win32':
			return 'windows';
		default:
			return arch;
	}
}

function download(opts) {
	const platform = nodePlatformToOS(opts.platform || os.platform());
	let arch = opts.arch || os.arch();
	if (arch === 'x64') {
		arch = 'amd64';
	}
	const version = opts.version;
	const symbols = opts.symbols || false;
	const filename = 'libui-' + version + '-' + platform + '-' + arch + '-shared' +
					 '.tgz';

	if (!version) {
		throw new Error('must specify needed version of libui in package.json');
	}
	const url = buildUrl(opts, filename);
	const cache = cacheDir(opts.cache);
	const actualCache = mkCacheDir(cache);

	debug('info', {cache: cache, filename: filename, url: url});

	let cachedZip = path.join(cache, filename);
	const exists = fs.existsSync(cachedZip);

	if (exists) {
		debug('zip exists', cachedZip);
		return Promise.resolve(cachedZip);
	}

	debug('creating cache/tmp dirs');

	// otherwise download it
	cachedZip = path.join(actualCache, filename); // in case cache dir changed

	// download to tmpdir
	var tmpdir =
		path.join(os.tmpdir(), 'libui-tmp-download-' + process.pid + '-' + Date.now());

	mkdirp.sync(tmpdir);
	debug(tmpdir + 'created');

	debug('downloading zip', url, 'to', tmpdir);

	const target = path.join(tmpdir, filename);

	return requestHttps(url)
		.then(resRedirect => {
			if (resRedirect.statusCode !== 302) {
				throw new Error(`Https request failed for ${url} with code ${
					resRedirect.statusCode}: resource not found.`);
			}
			return resRedirect;
		})
		.then(resRedirect => requestHttps(resRedirect.headers.location))
		.then(res => doDownload(res, url, target, cachedZip))
}

function main() {
	return download({version: process.env.npm_package_libui}).then(zipPath => {
		console.log('Downloaded zip:', zipPath);
		tar.extract({file: zipPath, sync: true});
		console.log('Libui binaries extracted to:', process.cwd());
	});
}

main().catch(err => {
	console.error(err.stack);
	process.exit(1);
});
