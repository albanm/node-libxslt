var fs = require('fs');

var should = require('should');
var libxmljs = require("libxmljs");

var libxslt = require('../index');

describe('node-libxslt bindings', function() {
	var stylesheetSource;
	before(function(callback){
		fs.readFile('./test/resources/cd.xsl', 'utf8', function(err, data){
			stylesheetSource = data;
			callback(err);
		});
	});
	var docSource;
	before(function(callback){
		fs.readFile('./test/resources/cd.xml', 'utf8', function(err, data){
			docSource = data;
			callback(err);
		});
	});

	var stylesheet;
	describe('stylesheet function', function() {
		it('should parse a stylesheet from a libxmljs xml document', function() {
			var stylesheetDoc = libxmljs.parseXml(stylesheetSource);
			stylesheet = libxslt.parse(stylesheetDoc);
			stylesheet.should.be.type('object');
		});
		it('should parse a stylesheet from a xml string', function() {
			stylesheet = libxslt.parse(stylesheetSource);
			stylesheet.should.be.type('object');
		});
		it('should throw an error when parsing invalid stylesheet', function() {
			(function() {
				libxslt.parse('this is not a stylesheet!');
			}).should.throw();
		});
	});

	describe('Synchronous Stylesheet.apply function', function() {
		it('should apply a stylesheet to a libxmljs xml document', function() {
			var doc = libxmljs.parseXml(docSource);
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
			var result = stylesheet.apply(docSource, {MyParam: 'MyParamValue'});
			result.should.be.type('string');
			result.should.match(/<p>My param: MyParamValue<\/p>/);
		});
	});

	// TODO Asynchronous implementation almost ok, but generates segfault.
	describe('Asynchronous Stylesheet.apply function', function() {
		it('should apply a stylesheet to a libxmljs xml document', function(callback) {
			var doc = libxmljs.parseXml(docSource);
			stylesheet.apply(doc, function(err, result){
				result.should.be.type('object');
				result.toString().should.match(/<td>Bob Dylan<\/td>/);
				callback();
			});
		});
		it('should apply a stylesheet to a xml string', function(callback) {
			stylesheet.apply(docSource, function(err, result){
				result.should.be.type('string');
				result.should.match(/<td>Bob Dylan<\/td>/);
				callback();
			});
		});
	});
	
});