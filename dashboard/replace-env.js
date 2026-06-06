const fs = require('fs');
const path = require('path');

const htmlPath = path.join(__dirname, 'index.html');
let html = fs.readFileSync(htmlPath, 'utf8');

html = html.replace(/__FIREBASE_API_KEY__/g, process.env.FIREBASE_API_KEY || '');
html = html.replace(/__FIREBASE_AUTH_DOMAIN__/g, process.env.FIREBASE_AUTH_DOMAIN || '');
html = html.replace(/__FIREBASE_DATABASE_URL__/g, process.env.FIREBASE_DATABASE_URL || '');
html = html.replace(/__FIREBASE_PROJECT_ID__/g, process.env.FIREBASE_PROJECT_ID || '');

fs.writeFileSync(htmlPath, html);
console.log('✅ Environment variables replaced in HTML');