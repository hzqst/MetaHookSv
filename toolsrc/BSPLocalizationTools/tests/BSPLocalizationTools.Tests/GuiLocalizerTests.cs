using System.Globalization;
using BSPLocalizationTools.GUI.Lang;

namespace BSPLocalizationTools.Tests;

public sealed class GuiLocalizerTests
{
    [Theory]
    [InlineData("en-US", "en-US")]
    [InlineData("zh-CN", "zh-CN")]
    [InlineData("zh-Hans", "zh-CN")]
    [InlineData("zh-SG", "zh-CN")]
    [InlineData("zh-TW", "zh-TW")]
    [InlineData("zh-Hant", "zh-TW")]
    [InlineData("zh-HK", "zh-TW")]
    [InlineData("fr-FR", "en-US")]
    public void ResolveSystemLanguageMapsSupportedCultures(string cultureName, string expected)
    {
        var actual = GuiLocalizer.ResolveEffectiveLanguageCode("auto", CultureInfo.GetCultureInfo(cultureName));

        Assert.Equal(expected, actual);
    }

    [Theory]
    [InlineData(TranslationStage.Queued, "已排队")]
    [InlineData(TranslationStage.ExtractingGameText, "正在提取 game_text")]
    [InlineData(TranslationStage.BuildingPrompt, "正在构建提示词")]
    [InlineData(TranslationStage.RequestingTranslation, "正在请求翻译")]
    [InlineData(TranslationStage.ParsingResponse, "正在解析响应")]
    [InlineData(TranslationStage.WritingDictionary, "正在写入词典")]
    [InlineData(TranslationStage.Completed, "已完成")]
    [InlineData(TranslationStage.Failed, "失败")]
    [InlineData(TranslationStage.Canceled, "已取消")]
    public void GetStageTextLocalizesTranslationStages(TranslationStage stage, string expected)
    {
        var localizer = new GuiLocalizer(() => CultureInfo.GetCultureInfo("en-US"));

        localizer.SetLanguage("zh-CN");

        Assert.Equal(expected, localizer.GetStageText(stage));
    }
}
