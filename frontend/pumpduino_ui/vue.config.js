// vue.config.js
var HtmlWebpackPlugin = require('html-webpack-plugin')
var HtmlWebpackInlineSourcePlugin = require('html-webpack-inline-source-plugin');

module.exports = {
    // assetsDir: './',
    filenameHashing: false,
    productionSourceMap: false,
    pages: {
        index: {
            // entry for the page
            entry: 'src/main.js',
            // the source template
            template: 'public/index.html',
            // output as dist/index.html
            filename: 'index.htm',
            // chunks to include on this page, by default includes
            // extracted common chunks and vendor chunks.
            // chunks: ['chunk-vendors', 'chunk-common', 'index']
        }
    },

    // configureWebpack: {
    //     plugins: [
    //         new HtmlWebpackPlugin({
    //             inlineSource: '.(js|css)$' // embed all javascript and css inline
    //         }),
    //         new HtmlWebpackInlineSourcePlugin(HtmlWebpackPlugin)
    //     ]
    // }
}