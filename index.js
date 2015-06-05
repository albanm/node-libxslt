/**
 * Node.js bindings for libxslt compatible with libxmljs
 * @module libxslt
 */

var fs = require('fs');
var libxmljs = require('libxmljs');
var binding = require('bindings')('node-libxslt');

binding.registerEXSLT();

/**
 * The libxmljs module. Prevents the need for a user's code to require it a second time. Also prevent weird bugs.
 */
exports.libxmljs = libxmljs;

/**
 * A compiled stylesheet. Do not call this constructor, instead use parse or parseFile.
 *
 * store both the source document and the parsed stylesheet
 * if we don't store the stylesheet doc it will be deleted by garbage collector and it will result in segfaults.
 *
 * @constructor
 * @param {Document} stylesheetDoc - XML document source of the stylesheet
 * @param {Document} stylesheetObj - Simple wrapper of a libxslt stylesheet
 */
var Stylesheet = function(stylesheetDoc, stylesheetObj){
	this.stylesheetDoc = stylesheetDoc;
	this.stylesheetObj = stylesheetObj;
};

/**
 * Parse a XSL stylesheet
 *
 * If no callback is given the function will run synchronously and return the result or throw an error.
 *
 * @param {string|Document} source - The content of the stylesheet as a string or a [libxmljs document]{@link https://github.com/polotek/libxmljs/wiki/Document}
 * @param {parseCallback} [callback] - The callback that handles the response. Expects err and Stylesheet object.
 * @return {Stylesheet} Only if no callback is given.
 */
exports.parse = function(source, callback) {
	// stylesheet can be given as a string or a pre-parsed xml document
	if (typeof source === 'string') {
		try {
			source = libxmljs.parseXml(source, { nocdata: true });
		} catch (err) {
			if (callback) return callback(err);
			throw err;
		}
	}
	
	if (callback) {
		binding.stylesheetAsync(source, function(err, stylesheet){
			if (err) return callback(err);
			callback(null, new Stylesheet(source, stylesheet));
		});
	} else {
		return new Stylesheet(source, binding.stylesheetSync(source));
	}
};
/**
 * Callback to the parse function
 * @callback parseCallback
 * @param {error} [err]
 * @param {Stylesheet} [stylesheet]
 */

/**
 * Parse a XSL stylesheet
 *
 * @param {stringPath} sourcePath - The path of the file
 * @param {parseFileCallback} callback - The callback that handles the response. Expects err and Stylesheet object.
 */
exports.parseFile = function(sourcePath, callback) {
	fs.readFile(sourcePath, 'utf8', function(err, data){
		if (err) return callback(err);
		exports.parse(data, callback);
	});
};
/**
 * Callback to the parseFile function
 * @callback parseFileCallback
 * @param {error} [err]
 * @param {Stylesheet} [stylesheet]
 */

/**
 * Register an extension function to be used in xpath expressions of a stylesheet
 * @param {string} name
 * @param {string} namespace
 * @param {function} fn
 */
exports.registerFunction = binding.registerFunction;

process.on("exit", binding.shutdownOnExit);

/**
 * Apply a stylesheet to a XML document
 *
 * If no callback is given the function will run synchronously and return the result or throw an error.
 *
 * @param {string|Document} source - The XML content to apply the stylesheet to given as a string or a [libxmljs document]{@link https://github.com/polotek/libxmljs/wiki/Document}
 * @param {object} [params] - Parameters passed to the stylesheet ({@link http://www.w3schools.com/xsl/el_with-param.asp})
 * @param {Stylesheet~applyCallback} [callback] - The callback that handles the response. Expects err and result of the same type as the source param passed to apply.
 * @return {string|Document} Only if no callback is given. Type is the same as the source param.
 */
Stylesheet.prototype.apply = function(source, params, callback) {
	// params are optional
	if (typeof params === 'function') {
		callback = params;
		params = {};
	}
	params = params || {};

	for(var p in params) {
		// string parameters must be surrounded by quotes to be usable by the stylesheet
		if (typeof params[p] === 'string') params[p] = '\'' + params[p] + '\'';
	}

	// xml can be given as a string or a pre-parsed xml document
	var outputString = false;
	if (typeof source === 'string') {
		try {
			source = libxmljs.parseXml(source);
		} catch (err) {
			if (callback) return callback(err);
			throw err;
		}
		outputString = true;
	}

	// flatten the params object in an array
	var paramsArray = [];
	for(var key in params) {
		paramsArray.push(key);
		paramsArray.push(params[key]);
	}

	// for some obscure reason I didn't manage to create a new libxmljs document in applySync,
	// but passing a document by reference and modifying its content works fine
	var result = new libxmljs.Document();

	if (callback) {
		binding.applyAsync(this.stylesheetObj, source, paramsArray, result, function(err){
			if (err) return callback(err);
			callback(null, outputString ? result.toString() : result);
		});
	} else {
		binding.applySync(this.stylesheetObj, source, paramsArray, result);	
		return outputString ? result.toString() : result;
	}
};
/**
 * Callback to the Stylesheet.apply function
 * @callback Stylesheet~applyCallback
 * @param {error} [err] - Error either from parsing the XML document if given as a string or from applying the styleshet
 * @param {string|Document} [result] Result of the same type as the source param passed to apply
 */

/**
 * Apply a stylesheet to a XML file
 *
 * @param {string} sourcePath - The path of the file to read
 * @param {object} [params] - Parameters passed to the stylesheet ({@link http://www.w3schools.com/xsl/el_with-param.asp})
 * @param {Stylesheet~applyToFileCallback} callback The callback that handles the response. Expects err and result as string.
 */
Stylesheet.prototype.applyToFile = function(sourcePath, params, callback) {
	var that = this;
	fs.readFile(sourcePath, 'utf8', function(err, data){
		if (err) return callback(err);
		that.apply(data, params, callback);
	});
};
/**
 * Callback to the Stylesheet.applyToFile function
 * @callback Stylesheet~applyToFileCallback
 * @param {error} [err] - Error either from reading the file, parsing the XML document or applying the styleshet
 * @param {string} [result]
 */
