using Avalonia;
using Avalonia.Controls;
using MetahookInstallerAvalonia.ViewModels;
using Ursa.Controls;

namespace MetahookInstallerAvalonia.Views;

public partial class MainView : UserControl
{
    public MainView()
    {
        InitializeComponent();
    }

    protected override void OnAttachedToVisualTree(VisualTreeAttachmentEventArgs e)
    {
        base.OnAttachedToVisualTree(e);
        if (DataContext is not MainViewModel vm)
            return;
        var topLevel = TopLevel.GetTopLevel(this);
        vm.NotificationManager = WindowNotificationManager.TryGetNotificationManager(topLevel, out var manager)
            ? manager
            : new WindowNotificationManager(topLevel);
        if (vm.NotificationManager != null)
            vm.NotificationManager.Position = Avalonia.Controls.Notifications.NotificationPosition.BottomLeft;
        vm.ToastManager = new WindowToastManager(TopLevel.GetTopLevel(this)) { MaxItems = 3 };
    }

    protected override void OnDetachedFromVisualTree(VisualTreeAttachmentEventArgs e)
    {
        base.OnDetachedFromVisualTree(e);
        if (DataContext is not MainViewModel vm)
            return;
        vm?.ToastManager?.Uninstall();
        vm?.NotificationManager?.Uninstall();
    }
}
