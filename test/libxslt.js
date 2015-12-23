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
		it('should apply a stylesheet to a libxslt.libxmljs xml document and force output as string', function() {
			var doc = libxslt.libxmljs.parseXml(docSource);
			var result = stylesheet.apply(doc, {}, {outputFormat: 'string'});
			result.should.be.type('string');
			result.should.match(/<td>Bob Dylan<\/td>/);
		});
		it('should apply a stylesheet to a xml string', function() {
			var result = stylesheet.apply(docSource);
			result.should.be.type('string');
			result.should.match(/<td>Bob Dylan<\/td>/);
		});
		it('should apply a stylesheet to a xml string and force output as document', function() {
			var result = stylesheet.apply(docSource, {}, {outputFormat: 'document'});
			result.should.be.type('object');
			result.toString().should.match(/<td>Bob Dylan<\/td>/);
		});
		it('should apply a stylesheet with a parameter', function() {
			var result = stylesheet.apply(docSource, {
				MyParam: 'MyParamValue'
			});
			result.should.be.type('string');
			result.should.match(/<p>My param: MyParamValue<\/p>/);
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
		it('should apply a stylesheet to a libxslt.libxmljs xml document and force output as string', function(callback) {
			var doc = libxslt.libxmljs.parseXml(docSource);
			stylesheet.apply(doc, {}, {outputFormat: 'string'}, function(err, result) {
				result.should.be.type('string');
				result.should.match(/<td>Bob Dylan<\/td>/);
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
		it('should apply a stylesheet to a xml string and force output as document', function(callback) {
			stylesheet.apply(docSource, {}, {outputFormat: 'document'}, function(err, result) {
				result.should.be.type('object');
				result.toString().should.match(/<td>Bob Dylan<\/td>/);
				callback();
			});
		});
		it('should apply a stylesheet with a include to a xml string', function(callback) {
			stylesheetInclude.apply(doc2Source, function(err, result) {
				result.should.be.type('string');
				result.should.match(/Title - Lover Birds/);
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

	describe('omit-xml-declaration directive', function() {
		it('should be respected by a stylesheet with output method text', function() {
			var data='<root><!-- comment on xml data --></root>';
			var stylesheetTextOut = libxslt.parse(fs.readFileSync('test/resources/omit-xml-declaration-text-out.xsl', 'utf8'));
			var result = stylesheetTextOut.apply(data);
			result.should.be.type('string');
			result.should.not.match(/\?xml/);
			result.should.match(/<foo\/>/);
			result.should.match(/<bar\/>/);
			result.should.not.match(/\<!-- comment/);
			result.should.not.match(/\<node/);
			result.should.match(/with text/);
	});

	it('should be respected by a stylesheet with output method xml', function() {
			var data='<root><!-- comment on xml data --></root>';
			var stylesheetXMLOut = libxslt.parse(fs.readFileSync('test/resources/omit-xml-declaration-xml-out.xsl', 'utf8'));
			var result = stylesheetXMLOut.apply(data);
			result.should.be.type('string');
			result.should.not.match(/\?xml/);
			result.should.match(/<foo\/>/);
			result.should.match(/&lt;bar\/&gt;/);
			result.should.not.match(/\<!-- comment/);
			result.should.match(/\<node/);
			result.should.match(/with text/);
		});
	});

	describe('implicitly omitted xml-declaration', function() {

		it('should be respected by a stylesheet with output method html', function() {
			var data='<root><strong></strong><!-- comment on xml data --></root>';
			var stylesheetHtmlOut = libxslt.parse(fs.readFileSync('test/resources/implicit-omit-xml-declaration-html-out.xsl', 'utf8'));
			var result = stylesheetHtmlOut.apply(data);
			result.should.be.type('string');
			result.should.not.match(/\?xml/);
			result.should.match(/<foo\/>/);
			result.should.match(/<strong><\/strong>/);
			result.should.match(/&lt;bar\/&gt;/);
			result.should.not.match(/\<!-- comment/);
			result.should.match(/\<node/);
			result.should.match(/with text/);
		});

		it('should be respected by a stylesheet with output method text', function() {
			var data='<root><strong>some text </strong><!-- comment on xml data --></root>';
			var stylesheetTextOut = libxslt.parse(fs.readFileSync('test/resources/implicit-omit-xml-declaration-text-out.xsl', 'utf8'));
			var result = stylesheetTextOut.apply(data);
			result.should.be.type('string');
			result.should.not.match(/\?xml/);
			result.should.match(/<foo\/>/);
			result.should.match(/<bar\/>/);
			result.should.not.match(/\<!-- comment/);
			result.should.not.match(/\<node/);
			result.should.not.match(/\<strong/);
			result.should.match(/some text with text/);
		});
	});

	describe('handle quotes in strings', function() {

		it('should avoid conflict with xpath single quote by using double-quotes', function() {
			var data='<root/>';
			var  xslDoc = libxslt.parse(fs.readFileSync('test/resources/handle-quotes-in-string-params.xsl', 'utf8'));
			var result = xslDoc.apply(data,{strParam:"/root/item[@id='123']"});
			result.should.match(/strParam:\/root\/item\[@id='123'\]/);
		});

	});

	describe('no params wrap', function() {

		it('should bypass parameter string wrap and deliver xpath expressions for nodesets', function() {
			var data='<root><item id="123">Ok 123</item><item id="321"/><other/></root>';
			var  xslDoc = libxslt.parse(fs.readFileSync('test/resources/use-xpath-params.xsl', 'utf8'));
			var result = xslDoc.apply(data,{at:"/root/item[@id='123']",testName:"'testing xpath selectors'"},{noWrapParams:true});
			result.should.match(/testing xpath selectors/);
			result.should.match(/#selected nodes:1/);
			result.should.match(/Node \[item id:123\] was selected./);
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
