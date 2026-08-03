<p align="center">
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/program_info/org.prismlauncher.PrismLauncher.logo-darkmode.svg">
  <source media="(prefers-color-scheme: light)" srcset="/program_info/org.prismlauncher.PrismLauncher.logo.svg">
  <img alt="Prism Launcher" src="/program_info/org.prismlauncher.PrismLauncher.logo.svg" width="40%">
</picture>
</p>

<p align="center">
  Prism Launcher 是一款用于 Minecraft 的自定义启动器，让你可以轻松同时管理多个 Minecraft 安装实例。<br />
  <br />它是 <b>MultiMC 启动器的一个分支（fork）</b>，且<b>未</b>得到 MultiMC 的背书。
</p>

## 安装

- 所有下载与安装说明都可以在我们的[官网](https://prismlauncher.org/download)找到。
- 最新的构建状态可以在 [GitHub Actions](https://github.com/PrismLauncher/PrismLauncher/actions) 标签页查看（其中也包含 Pull Request 的构建状态）。

<p align="center">
<a href="https://repology.org/project/prismlauncher/versions">
    <img src="https://repology.org/badge/vertical-allrepos/prismlauncher.svg?columns=3" alt="打包状态">
</a>
</p>

### 开发版构建

请注意，这些构建并非面向大多数用户。其中可能存在 Bug，以及其他不稳定因素。你已经收到警告了。

我们提供以下渠道的开发版构建：

- [GitHub Actions](https://github.com/PrismLauncher/PrismLauncher/actions)（包含贡献者提交的 Pull Request 构建）
- [nightly.link](https://prismlauncher.org/nightly)（始终仅指向 develop 分支的最新版本）

这些构建的二进制文件中包含调试信息，因此文件体积相对更大。

我们为 **Linux**、**Windows** 和 **macOS** 提供预编译的开发版构建。

在 Linux 上，我们还提供自己的 [Flatpak 每日构建仓库](https://github.com/PrismLauncher/flatpak)。大多数软件中心都可以通过打开[此链接](https://flatpak.prismlauncher.org/prismlauncher-nightly.flatpakref)来安装它。

## 社区与支持

如果你发现了 Bug 或想建议新功能，欢迎随时创建 GitHub Issue。我们有多个社区空间，其他社区成员可以在那里帮助你：

- **我们的 Discord 服务器：**

[![Prism Launcher Discord 服务器](https://discordapp.com/api/guilds/1031648380885147709/widget.png?style=banner3)](https://prismlauncher.org/discord)

- **我们的 Matrix 空间：**

[![Prism Launcher 空间](https://img.shields.io/matrix/prismlauncher:matrix.org?style=for-the-badge&label=Matrix%20Space&logo=matrix&color=purple)](https://prismlauncher.org/matrix)

- **我们的 Subreddit 版块：**

[![r/PrismLauncher](https://img.shields.io/reddit/subreddit-subscribers/prismlauncher?style=for-the-badge&logo=reddit)](https://prismlauncher.org/reddit)

## 翻译

Prism Launcher 的翻译工作托管在 [Weblate](https://hosted.weblate.org/projects/prismlauncher/launcher/)，关于翻译 Prism Launcher 的信息可在 <https://github.com/PrismLauncher/Translations> 查看。

## 构建

如果你想自行构建 Prism Launcher，请查看[构建说明](https://prismlauncher.org/wiki/development/build-instructions)。

## 赞助商与合作伙伴

我们感谢 [Open Collective](https://opencollective.com/prismlauncher) 上所有优秀的支持者！你可以通过[成为支持者](https://opencollective.com/prismlauncher)来支持 Prism Launcher。

[![OpenCollective 支持者](https://opencollective.com/prismlauncher/backers.svg?width=890&limit=1000)](https://opencollective.com/prismlauncher#backers)

感谢 JetBrains 通过他们的[开源计划](https://www.jetbrains.com/opensource/)为我们提供其全系列产品的若干授权许可。

<a href="https://jb.gg/OpenSource">
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://www.jetbrains.com/company/brand/img/logo_jb_dos_4.svg">
  <source media="(prefers-color-scheme: light)" srcset="https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg">
  <img alt="JetBrains 标志" src="https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg" width="40%">
</picture>
</a>

感谢 Weblate 托管我们的翻译工作。

<a href="https://hosted.weblate.org/engage/prismlauncher/">
<img src="https://hosted.weblate.org/widgets/prismlauncher/-/open-graph.png" alt="翻译状态" width="300" />
</a>

感谢 Netlify 通过他们的[开源计划](https://www.netlify.com/open-source/)为我们提供出色的 Web 服务。

<a href="https://www.netlify.com"> <img src="https://www.netlify.com/v3/img/components/netlify-color-accent.svg" alt="由 Netlify 部署" /> </a>

感谢 [MacStadium](https://www.macstadium.com/) 的出色团队为开发目的提供 M1 Mac！

<a href="https://www.macstadium.com"><img src="https://uploads-ssl.webflow.com/5ac3c046c82724970fc60918/5c019d917bba312af7553b49_MacStadium-developerlogo.png" alt="由 MacStadium 提供支持" width="300"></a>

## 分支 / 再分发 / 自定义构建政策

你可以自由地分支（fork）、再分发以及提供自定义构建，前提是遵守[许可证](LICENSE)的条款（这是一项法律责任）；并且，如果你做的是代码修改而非仅仅打包一个自定义构建，请出于基本礼节做到以下几点：

- 明确说明你的分支不是 Prism Launcher，且未得到 Prism Launcher 项目（<https://prismlauncher.org>）的认可或与之有关联。
- 通读 [CMakeLists.txt](CMakeLists.txt)，将 Prism Launcher 的 API 密钥替换为你自己的，或者将它们设为空字符串（`""`）以禁用它们（这样程序仍能编译，但需要这些密钥的功能会被禁用）。

如果你对上述条件有任何疑问或想要澄清，请创建 Issue 向我们提问。

如果你只是为你的发行版构建 Prism Launcher，请确保将 `Launcher_BUILD_PLATFORM` 设置为代表你发行版的 slug。例如 `archlinux`、`fedora` 和 `nixpkgs`。

请注意，如果你在构建本软件时未移除 [CMakeLists.txt](CMakeLists.txt) 中提供的 API 密钥，即表示你接受以下条款与条件：

- [Microsoft 身份平台使用条款](https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use)
- [CurseForge 第三方 API 条款与条件](https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions)

如果你不同意这些条款与条件，请将 [CMakeLists.txt](CMakeLists.txt) 文件中相关的 API 密钥设为空字符串（`""`）以移除它们。

## 许可证 [![https://github.com/PrismLauncher/PrismLauncher/blob/develop/LICENSE](https://img.shields.io/github/license/PrismLauncher/PrismLauncher?label=License&logo=gnu&color=C4282D)](LICENSE)

所有启动器代码均基于 GPL-3.0-only 许可证发布。

标志及相关素材基于 CC BY-SA 4.0 许可证发布。
