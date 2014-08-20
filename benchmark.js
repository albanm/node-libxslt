var fs = require('fs');
var async = require('async');
var libxmljs = require("libxmljs");
var libxslt = require('./index');

var stylesheetStr = fs.readFileSync('./test/resources/cd.xsl', 'utf8');
var stylesheetObj = libxmljs.parseXml(stylesheetStr);
var stylesheet = libxslt.parse(stylesheetObj);
var docStr = fs.readFileSync('./test/resources/cd.xml', 'utf8');
var docObj = libxmljs.parseXml(docStr);

var bench = function(name, iterations, f) {
	return function(callback) {
		var before = Date.now();
		f(iterations, function() {
			var duration = (Date.now() - before);
			console.log('%d %s in %dms = %d/s', iterations, name, duration, Math.round(iterations / (duration / 1000)));
			if (callback) callback();
		});
	};
};

var stylesheetParsingStr = function(iterations, callback) {
	for (var i = 0; i < iterations; i++) {
		libxslt.parse(stylesheetStr);
	}
	callback();
};

var stylesheetParsingObj = function(iterations, callback) {
	for (var i = 0; i < iterations; i++) {
		libxslt.parse(stylesheetObj);
	}
	callback();
};

var applySyncStr = function(iterations, callback) {
	for (var i = 0; i < iterations; i++) {
		stylesheet.apply(docStr);
	}
	callback();
};

var applySyncObj = function(iterations, callback) {
	for (var i = 0; i < iterations; i++) {
		stylesheet.apply(docObj);
	}
	callback();
};

var applyAsyncSeriesStr = function(iterations, callback) {
	var i = 0;
	async.eachSeries(new Array(iterations), function(u, callbackEach) {
		stylesheet.apply(docStr, function(err, result) {
			i++;
			callbackEach(err);
		});
	}, callback); 
};

var applyAsyncSeriesObj = function(iterations, callback) {
	var i = 0;
	async.eachSeries(new Array(iterations), function(u, callbackEach) {
		stylesheet.apply(docObj, function(err, result) {
			i++;
			callbackEach(err);
		});
	}, callback); 
};

var applyAsyncParallelStr = function(iterations, callback) {
	var i = 0;
	async.eachLimit(new Array(iterations), 10, function(u, callbackEach) {
		stylesheet.apply(docStr, function(err, result) {
			i++;
			callbackEach(err);
		});
	}, callback); 
};

var applyAsyncParallelObj = function(iterations, callback) {
	var i = 0;
	async.eachLimit(new Array(iterations), 10, function(u, callbackEach) {
		stylesheet.apply(docObj, function(err, result) {
			i++;
			callbackEach(err);
		});
	}, callback); 
};

var iterations = 10000;
async.series([
	//bench('stylesheet parsing from string\t\t\t', iterations, stylesheetParsingStr),
	//bench('stylesheet parsing from parsed doc\t\t\t', iterations, stylesheetParsingObj),
	//bench('synchronous apply from string\t\t\t', iterations, applySyncStr),
	bench('synchronous apply from parsed doc\t\t\t', iterations, applySyncObj),
	//bench('asynchronous apply in series from string\t\t', iterations, applyAsyncSeriesStr),
	bench('asynchronous apply in series from parsed doc\t', iterations, applyAsyncSeriesObj),
	//bench('asynchronous apply in parallel from string\t', iterations, applyAsyncParallelStr),
	bench('asynchronous apply in parallel from parsed doc\t', iterations, applyAsyncParallelObj)
]);