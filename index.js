var libxmljs = require('libxmljs');
var binding = require('./build/Debug/node-libxslt');
//var binding = require('./build/Release/node-libxslt');

var Stylesheet = function(stylesheetObj){
	this.stylesheetObj = stylesheetObj;
};
Stylesheet.prototype.apply = function(source, params, callback) {
	// xml can be given as a string or a pre-parsed xml document
	var outputString = false;
	if (typeof source === 'string') {
		source = libxmljs.parseXml(source);
		outputString = true;
	}
	if (typeof params === 'function') {
		callback = params;
		params = {};
	}
	params = params || {};

	// flatten the params object in an array
	var paramsArray = [];
	for(var key in params) {
		paramsArray.push(key);
		paramsArray.push(params[key]);
	}

	// for some obscure reason I didn't manage to create a new libxmljs document in applySync,
	// but passing a document by reference and modifying its content works fine
	var result = new libxmljs.Document();

	binding.applySync(this.stylesheetObj, source, params, result);

	return outputString ? result.toString() : result;
};

exports.stylesheet = function(source, callback) {
	// stylesheet can be given as a string or a pre-parsed xml document
	console.log(1);
	if (typeof source === 'string') source = libxmljs.parseXml(source);
	console.log(2);
	
	if (callback) {
		//return binding.stylesheetAsync(source);
	} else {
		console.log(3);
		var s = new Stylesheet(binding.stylesheetSync(source));
		console.log(4);
		return s;
	}
};
