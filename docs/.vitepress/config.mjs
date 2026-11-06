import {defineConfig} from 'vitepress'
// https://vitepress.dev/reference/site-config
export default defineConfig({
    title : "HelpMake",
    description : "A C/C++ build system that needs no config file",
    base : '/HelpMake/',
    ignoreDeadLinks : true,

    sitemap : {
        hostname : 'https://Gag91.github.io/HelpMake/'
    },

    head : [
        [ 'meta', {name : 'google-site-verification', content : 'i5PX85IQ7kcRa66R_Kjmevuu-Q4CnOGZWaPY5NoohAU'} ]
    ],

    markdown : {
        html : false
    },

    themeConfig : {
        logo : '/logo.png',

        nav : [
            {text : 'Home', link : '/'},
            {text : 'Docs', link : '/01-getting-started'},
            {text : 'Install', link : '/install'},
            {text : 'GitHub', link : 'https://github.com/Gag91/Help_Make'}
        ],

        sidebar : [
            {

                text : 'Guide',
                items : [
                    {text : 'Installation', link : '/install'},
                    {text : 'Getting Started', link : '/01-getting-started'},
                    {text : 'Auto-detection', link : '/11-auto-detection'},
                    {text : 'Variables', link : '/05-variables'},
                ]

            },
            {
                text : 'Reference',
                items : [
                    {text : 'HelpMake.txt syntax', link : '/02-helpmake-reference'},
                    {text : 'CLI reference', link : '/03-cli-reference'},
                    {text : 'Toolchains', link : '/04-toolchains'},
                    {text : 'Build directory', link : '/09-build-directory'},
                ]
            },
            {
                text : 'Features',
                items : [
                    {text : 'Precompiled headers', link : '/06-pch'},
                    {text : 'C++20 modules', link : '/07-modules'},
                    {text : 'Static libraries', link : '/08-static-libs'},
                ]
            },
            {
                text : 'Other',
                items : [
                    {text : 'FAQ', link : '/10-faq'},
                    {text : 'Changelog', link : '/Change_Logs'},
                ]
            }
        ],

        search : {
            provider : 'local'
        },

        editLink : {
            pattern : 'https://github.com/Gag91/Help_Make/edit/main/docs/:path',
            text : 'Edit this page on GitHub'
        },

        socialLinks : [
            {icon : 'github', link : 'https://github.com/Gag91/Help_Make'}
        ],

        footer : {
            message : 'Released under the MIT License.',
            copyright : 'Copyright © 2026 Xavi99'
        }
    }
})