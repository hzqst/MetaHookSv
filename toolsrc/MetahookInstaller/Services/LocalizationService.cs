using System;
using System.Globalization;
using System.Resources;
using System.Windows;
using MetahookInstaller.Resources;

namespace MetahookInstaller.Services
{
    public class LocalizationService
    {
        private static readonly ResourceManager ResourceManager = new ResourceManager("MetahookInstaller.Resources.Strings", typeof(Strings).Assembly);

        public static string GetString(string key, params object[] args)
        {
            try
            {
                var culture = CultureInfo.CurrentUICulture;
                var value = ResourceManager.GetString(key, culture);
                
                // 如果找不到资源，返回key作为后备
                if (string.IsNullOrEmpty(value))
                {
                    return key;
                }

                // 如果没有参数，直接返回字符串
                if (args == null || args.Length == 0)
                {
                    return value;
                }

                // 格式化字符串
                return string.Format(value, args);
            }
            catch (Exception)
            {
                return key;
            }
        }

        public static void Initialize()
        {
            // 设置默认语言为英文
            var defaultCulture = new CultureInfo("en-US");
            
            // 如果系统语言是简体中文，则使用中文资源
            if (CultureInfo.CurrentUICulture.Name == "zh-CN")
            {
               defaultCulture = new CultureInfo("zh-CN");
            }
            
            CultureInfo.CurrentUICulture = defaultCulture;
            CultureInfo.CurrentCulture = defaultCulture;
        }
    }
} 