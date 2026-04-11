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
				'Local offline voice input for Fcitx5 with on-device ASR, optional LLM post-processing, and cross-distro packaging.',
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
			customCss: ['./src/styles/custom.css'],
			sidebar: ['overview', 'downloads', 'documentation'],
		}),
	],
});
