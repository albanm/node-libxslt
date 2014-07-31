// Mostly to run for debugging purposes
var fs = require('fs');
var libxmljs = require("libxmljs");
var libxslt = require('./index');

var stylesheetSource = fs.readFileSync('./test/cd.xsl', 'utf8');
//var docSource = fs.readFileSync('./test/cd.xml', 'utf8');

var stylesheet = libxslt.stylesheet(stylesheetSource);
//var result = stylesheet.apply(docSource);

//console.log(result.toString());