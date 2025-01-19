const fs = require('fs');
const path = require('path');
const uglifyJS = require('uglify-js');
const cleanCSS = require('clean-css');
const htmlMinifier = require('html-minifier');
const zlib = require('zlib');

const STATIC_DIR = path.join(__dirname, '../static/web');
const DATA_DIR = path.join(__dirname, '../data/web');

function ensureDirectoryExists(dirPath) {
    if (!fs.existsSync(dirPath)) {
        fs.mkdirSync(dirPath, { recursive: true });
    }
}

function getRelativePath(filepath) {
    return path.relative(STATIC_DIR, path.dirname(filepath));
}

function processJS(filepath) {
    const content = fs.readFileSync(filepath, 'utf8');
    const relPath = getRelativePath(filepath);
    const filename = path.basename(filepath);
    const outputDir = path.join(DATA_DIR, relPath);

    ensureDirectoryExists(outputDir);

    const result = uglifyJS.minify(content, {
        sourceMap: false  // No need for source map if we're only using gz
    });

    if (result.error) throw result.error;

    // Only write gzipped version
    fs.writeFileSync(
        path.join(outputDir, `${filename}.gz`),
        zlib.gzipSync(result.code)
    );
}

function processCSS(filepath) {
    const content = fs.readFileSync(filepath, 'utf8');
    const relPath = getRelativePath(filepath);
    const filename = path.basename(filepath);
    const outputDir = path.join(DATA_DIR, relPath);

    ensureDirectoryExists(outputDir);

    const result = new cleanCSS().minify(content);

    // Only write gzipped version
    fs.writeFileSync(
        path.join(outputDir, `${filename}.gz`),
        zlib.gzipSync(result.styles)
    );
}

function processHTML(filepath) {
    const content = fs.readFileSync(filepath, 'utf8');
    const relPath = getRelativePath(filepath);
    const filename = path.basename(filepath);
    const outputDir = path.join(DATA_DIR, relPath);

    ensureDirectoryExists(outputDir);

    const minified = htmlMinifier.minify(content, {
        collapseWhitespace: true,
        removeComments: true,
        minifyJS: true,
        minifyCSS: true
    });

    // Only write gzipped version
    fs.writeFileSync(
        path.join(outputDir, `${filename}.gz`),
        zlib.gzipSync(minified)
    );
}

function processOtherFiles(filepath) {
    const content = fs.readFileSync(filepath);
    const relPath = getRelativePath(filepath);
    const filename = path.basename(filepath);
    const outputDir = path.join(DATA_DIR, relPath);

    ensureDirectoryExists(outputDir);

    // Only write gzipped version
    fs.writeFileSync(
        path.join(outputDir, `${filename}.gz`),
        zlib.gzipSync(content)
    );
}

function removeDirectory(dir) {
    if (fs.existsSync(dir)) {
        fs.rmSync(dir, { recursive: true });
    }
}

function processDirectory(dir) {
    const files = fs.readdirSync(dir);

    files.forEach(file => {
        const filepath = path.join(dir, file);
        const stat = fs.statSync(filepath);

        if (stat.isDirectory()) {
            processDirectory(filepath);
            return;
        }

        const ext = path.extname(file).toLowerCase();

        console.log(`Processing: ${path.relative(STATIC_DIR, filepath)}`);

        switch (ext) {
            case '.js':
                processJS(filepath);
                break;
            case '.css':
                processCSS(filepath);
                break;
            case '.html':
                processHTML(filepath);
                break;
            default:
                processOtherFiles(filepath);
        }
    });
}

// Start processing
console.log('Starting static assets build...');

// Remove existing data directory
console.log('Cleaning data directory...');
removeDirectory(DATA_DIR);
ensureDirectoryExists(DATA_DIR);

// Process all files
processDirectory(STATIC_DIR);
console.log('Build complete! Files are in the data directory.');