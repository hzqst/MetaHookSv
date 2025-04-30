namespace MetahookInstaller.Models
{
    public class GameInfo
    {
        public string Name { get; set; }        // 显示名称
        public string ShortName { get; set; }   // 短名称
        public uint AppId { get; set; }         // Steam AppId
        public string ModName { get; set; }     // 对应的mod名称

        public GameInfo(string name, string shortName, uint appId, string modName)
        {
            Name = name;
            ShortName = shortName;
            AppId = appId;
            ModName = modName;
        }
    }
} 