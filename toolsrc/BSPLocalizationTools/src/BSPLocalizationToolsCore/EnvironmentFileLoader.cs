namespace BSPLocalizationTools;

public static class EnvironmentFileLoader
{
    public static void LoadFromCurrentDirectory()
    {
        DotNetEnv.Env.NoClobber().TraversePath().Load();
    }
}
