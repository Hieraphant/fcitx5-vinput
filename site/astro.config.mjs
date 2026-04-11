// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
	site: 'https://xifan2333.github.io',
	base: '/fcitx5-vinput',
	integrations: [
		starlight({
			title: 'fcitx5-vinput',
			logo: {
				src: './public/favicon.svg',
			},
			description:
				'Voice input for Fcitx5 with local and cloud ASR, LLM rewriting, and cross-distro installation.',
			favicon: '/favicon.svg',
			defaultLocale: 'root',
			locales: {
				root: {
					label: 'English',
					lang: 'en',
				},
				'zh-cn': {
					label: '简体中文',
					lang: 'zh-CN',
				},
			},
			social: [
				{
					icon: 'github',
					label: 'GitHub',
					href: 'https://github.com/xifan2333/fcitx5-vinput',
				},
			],
			editLink: {
				baseUrl:
					'https://github.com/xifan2333/fcitx5-vinput/edit/main/site/',
			},
			components: {
				SiteTitle: './src/components/SiteTitle.astro',
			},
			head: [
				{
					tag: 'link',
					attrs: {
						rel: 'preconnect',
						href: 'https://fonts.googleapis.com',
					},
				},
				{
					tag: 'link',
					attrs: {
						rel: 'preconnect',
						href: 'https://fonts.gstatic.com',
						crossorigin: '',
					},
				},
				{
					tag: 'link',
					attrs: {
						rel: 'stylesheet',
						href: 'https://fonts.googleapis.com/css2?family=Audiowide&display=swap',
					},
				},
			],
			customCss: ['./src/styles/custom.css'],
			sidebar: ['overview', 'downloads', 'documentation'],
		}),
	],
});
