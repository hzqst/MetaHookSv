using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.VisualTree;
using Avalonia.Xaml.Interactions.DragAndDrop;
using MetahookInstallerAvalonia.ViewModels;
using System.Collections.ObjectModel;

namespace MetahookInstallerAvalonia.Behavior;

public class ItemsListBoxDropHandler : DropHandlerBase
{
    private bool Validate<T>(ListBox listBox, DragEventArgs e, object? sourceContext, object? targetContext, bool bExecute) where T : PluginInfo
    {
        if (sourceContext is not T sourceItem
            || targetContext is not MainViewModel vm
            || listBox.GetVisualAt(e.GetPosition(listBox)) is not Control targetControl
            || targetControl.DataContext is not T targetItem)
        {
            return false;
        }

        var items = vm.Plugins;
        var sourceIndex = items.IndexOf(sourceItem);
        var targetIndex = items.IndexOf(targetItem);

        if (sourceIndex < 0 || targetIndex < 0)
        {
            return false;
        }

        switch (e.DragEffects)
        {
            case DragDropEffects.Copy:
                {
                    if (bExecute)
                    {
                        var clone = sourceItem.Clone();
                        InsertItem(items, clone, targetIndex + 1);
                        vm.RecaculatePluginIndex();
                    }
                    return true;
                }
            case DragDropEffects.Move:
                {
                    if (bExecute)
                    {
                        MoveItem(items, sourceIndex, targetIndex);
                        vm.RecaculatePluginIndex();
                    }
                    return true;
                }
            case DragDropEffects.Link:
                {
                    if (bExecute)
                    {
                        SwapItem(items, sourceIndex, targetIndex);
                        vm.RecaculatePluginIndex();
                    }
                    return true;
                }
            default:
                return false;
        }
    }

    public override bool Validate(object? sender, DragEventArgs e, object? sourceContext, object? targetContext, object? state)
    {
        if (e.Source is Control && sender is ListBox listBox)
        {
            return Validate<PluginInfo>(listBox, e, sourceContext, targetContext, false);
        }
        return false;
    }

    public override bool Execute(object? sender, DragEventArgs e, object? sourceContext, object? targetContext, object? state)
    {
        if (e.Source is Control && sender is ListBox listBox)
        {
            return Validate<PluginInfo>(listBox, e, sourceContext, targetContext, true);
        }
        return false;
    }
}