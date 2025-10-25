using System;
using System.Collections;
using System.Globalization;
using System.Collections.Generic;
using Avalonia.Data.Converters;

namespace MetahookInstallerAvalonia.Converters;

public class ItemIndexConverter : IMultiValueConverter
{
    public object? Convert(IList<object?> values, Type targetType, object? parameter, CultureInfo culture)
    {
        if (values == null || values.Count < 2)
            return string.Empty;

        var item = values[1];
        if (values[0] is not IEnumerable items || item == null)
            return string.Empty;

        int i = 0;
        foreach (var it in items)
        {
            if (ReferenceEquals(it, item) || Equals(it, item))
                return (i + 1).ToString();
            i++;
        }

        return string.Empty;
    }
}