var fs = require('fs');

var should = require('should');

var libxslt = require('../index');

describe('node-libxslt', function() {
	var stylesheetSource;
	before(function(callback) {
		fs.readFile('./test/resources/cd.xsl', 'utf8', function(err, data) {
			stylesheetSource = data;
			callback(err);
		});
	});
	var docSource;
	before(function(callback) {
		fs.readFile('./test/resources/cd.xml', 'utf8', function(err, data) {
			docSource = data;
			callback(err);
		});
	});

	var stylesheetIncludeSource;
	before(function(callback) {
		fs.readFile('./test/resources/xslinclude.xsl', 'utf8', function(err, data) {
			stylesheetIncludeSource = data;
			callback(err);
		});
	});

	var doc2Source;
	before(function(callback) {
		fs.readFile('./test/resources/collection.xml', 'utf8', function(err, data) {
			doc2Source = data;
			callback(err);
		});
	});

	var stylesheet;
	var stylesheetInclude;
	describe('synchronous parse function', function() {
		it('should parse a stylesheet from a libxslt.libxmljs xml document', function() {
			var stylesheetDoc = libxslt.libxmljs.parseXml(stylesheetSource);
			stylesheet = libxslt.parse(stylesheetDoc);
			stylesheet.should.be.type('object');
		});
		it('should parse a stylesheet from a xml string', function() {
			stylesheet = libxslt.parse(stylesheetSource);
			stylesheet.should.be.type('object');
		});
		it('should parse a stylesheet with a include from a xml string', function() {
			var stylesheetDoc = libxslt.libxmljs.parseXml(stylesheetIncludeSource);
			stylesheetInclude = libxslt.parse(stylesheetDoc);
			stylesheetInclude.should.be.type('object');
		});
		it('should throw an error when parsing invalid stylesheet', function() {
			(function() {
				libxslt.parse('this is not a stylesheet!');
			}).should.throw();
		});
	});

	describe('parseFile function', function() {
		it('should parse a stylesheet from a file', function(callback) {
			libxslt.parseFile('./test/resources/cd.xsl', function(err, stylesheet) {
				stylesheet.should.be.type('object');
				callback(err);
			});
		});
	});

	describe('asynchronous parse function', function() {
		it('should parse a stylesheet from a libxslt.libxmljs xml document', function(callback) {
			var stylesheetDoc = libxslt.libxmljs.parseXml(stylesheetSource);
			libxslt.parse(stylesheetDoc, function(err, stylesheet) {
				stylesheet.should.be.type('object');
				callback(err);
			});
		});
		it('should parse a stylesheet from a xml string', function(callback) {
			libxslt.parse(stylesheetSource, function(err, stylesheet) {
				stylesheet.should.be.type('object');
				callback(err);
			});
		});
		it('should parse a stylesheet with a include from a xml string', function(callback) {
			var stylesheetDoc = libxslt.libxmljs.parseXml(stylesheetIncludeSource);
			libxslt.parse(stylesheetDoc, function(err, stylesheet) {
				stylesheet.should.be.type('object');
				callback(err);
			});
		});
		it('should return an error when parsing invalid stylesheet', function(callback) {
			libxslt.parse('this is not a stylesheet!', function(err) {
				should.exist(err);
				callback();
			});
		});
	});

	describe('synchronous apply function', function() {
		it('should apply a stylesheet to a libxslt.libxmljs xml document', function() {
			var doc = libxslt.libxmljs.parseXml(docSource);
			var result = stylesheet.apply(doc);
			result.should.be.type('object');
			result.toString().should.match(/<td>Bob Dylan<\/td>/);
		});
		it('should apply a stylesheet to a xml string', function() {
			var result = stylesheet.apply(docSource);
			result.should.be.type('string');
			result.should.match(/<td>Bob Dylan<\/td>/);
		});
		it('should apply a stylesheet with a parameter', function() {
			var result = stylesheet.apply(docSource, {
				MyParam: 'MyParamValue'
			});
			result.should.be.type('string');
			result.should.match(/<p>My param: MyParamValue<\/p>/);
		});
		it('should apply a stylesheet with a include to a xml string', function(callback) {
			stylesheetInclude.apply(doc2Source, function(err, result) {
				result.should.be.type('string');
				result.should.match(/Title - Lover Birds/);
				callback();
			});
		});
	});

	describe('asynchronous apply function', function() {
		it('should apply a stylesheet to a libxslt.libxmljs xml document', function(callback) {
			var doc = libxslt.libxmljs.parseXml(docSource);
			stylesheet.apply(doc, function(err, result) {
				result.should.be.type('object');
				result.toString().should.match(/<td>Bob Dylan<\/td>/);
				callback();
			});
		});
		it('should apply a stylesheet to a xml string', function(callback) {
			stylesheet.apply(docSource, function(err, result) {
				result.should.be.type('string');
				result.should.match(/<td>Bob Dylan<\/td>/);
				callback();
			});
		});
	});

	describe('applyToFile function', function() {
		it('should apply a stylesheet to a xml file', function(callback) {
			stylesheet.applyToFile('./test/resources/cd.xml', function(err, result) {
				result.should.be.type('string');
				result.should.match(/<td>Bob Dylan<\/td>/);
				callback();
			});
		});
	});

	describe('disable-output-escaping attribute', function() {
		it('should be interpreted by a stylesheet', function() {
			var stylesheetStr = fs.readFileSync('test/resources/disable-output-escaping.xsl', 'utf8');
			var stylesheetEsc = libxslt.parse(stylesheetStr);
			var result = stylesheetEsc.apply('<root/>');
			result.should.match(/<foo\/>/);
			result.should.match(/&lt;bar\/&gt;/);
		});
	});

	describe('libexslt bindings', function(){
		it('should expose EXSLT functions', function(callback){
			libxslt.parseFile('test/resources/min-value.xsl', function(err, stylesheet){
				should.not.exist(err);
				stylesheet.applyToFile('test/resources/values.xml', function(err, result){
					should.not.exist(err);
					result.should.match(/Minimum: 4/);
					callback();
				});
			});
		});
	});
});