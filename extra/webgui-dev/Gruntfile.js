/*global module */

module.exports = function(grunt) {
    grunt.initConfig({
	pkg: grunt.file.readJSON('package.json'),
	concat: {
	    options: {
		sourceMap: true
	    },
	    dist: {
		src: ['src/banner.js','src/Nanocube3/*.js','src/footer.js'],
		dest: 'dist/<%= pkg.name %>.js'
	    }
	},
	jshint: {
	    options: {ignores: ['src/banner.js', 'src/footer.js']},
	    files: ['Gruntfile.js', 'src/**/*.js']
	}
    });

    grunt.loadNpmTasks('grunt-contrib-jshint');
    grunt.loadNpmTasks('grunt-contrib-concat');

    grunt.registerTask('default', ['concat','jshint']);
};
