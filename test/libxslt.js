var fs = require('fs');

var should = require('should');
var libxmljs = require("libxmljs");

var libxslt = require('../index');

describe('node-libxslt bindings', function() {
	var stylesheetSource;
	before(function(){
		fs.readFile('./test/resources/cd.xsl', 'utf8', function(err, data){
			if(err) throw(err);
			stylesheetSource = data;
		});
	});
	var docSource;
	before(function(){
		fs.readFile('./test/resources/cd.xml', 'utf8', function(err, data){
			if(err) throw(err);
			docSource = data;
		});
	});

	var stylesheet;
	describe('stylesheet function', function() {
		it('should parse a stylesheet from a libxmljs xml document', function() {
			var stylesheetDoc = libxmljs.parseXml(stylesheetSource);
			stylesheet = libxslt.stylesheet(stylesheetDoc);
			stylesheet.should.be.type('object');
		});
		it('should parse a stylesheet from a xml string', function() {
			stylesheet = libxslt.stylesheet(stylesheetSource);
			stylesheet.should.be.type('object');
		});
		it('should throw an error when parsing invalid stylesheet', function() {
			(function() {
				libxslt.stylesheet('this is not a stylesheet!');
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