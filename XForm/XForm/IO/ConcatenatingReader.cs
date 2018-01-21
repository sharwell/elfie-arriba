﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;

using XForm.Data;
using XForm.Extensions;

namespace XForm.IO
{
    /// <summary>
    ///  ConcatenatingReader groups together multiple IXTable sources and
    ///  returns the rows from them one by one, showing the union of all available columns.
    /// </summary>
    //public class ConcatenatingReader : IXTable
    //{
    //    private IXTable[] _sources;
    //    private List<ColumnDetails> _columns;
    //    private int _currentSourceIndex;

    //    public ConcatenatingReader(IEnumerable<IXTable> sources)
    //    {
    //        _sources = sources.ToArray();
    //        _columns = new List<ColumnDetails>();
    //        _currentSourceIndex = 0;

    //        IdentifyColumns();
    //    }

    //    private void IdentifyColumns()
    //    {
    //        // Find the union of all columns. Ensure the names match
    //        Dictionary<string, ColumnDetails> columns = new Dictionary<string, ColumnDetails>(StringComparer.OrdinalIgnoreCase);
    //        foreach (IXTable source in _sources)
    //        {
    //            foreach (ColumnDetails column in source.Columns)
    //            {
    //                ColumnDetails existingColumn;
    //                if (columns.TryGetValue(column.Name, out existingColumn))
    //                {
    //                    if (!column.Equals(existingColumn))
    //                    {
    //                        throw new ArgumentException($"ConcatenatingReader couldn't combine sources because column {column.Name} had different types from different sources.");
    //                    }
    //                }
    //                else
    //                {
    //                    columns.Add(column.Name, column);
    //                    _columns.Add(column);
    //                }
    //            }
    //        }
    //    }

    //    public IReadOnlyList<ColumnDetails> Columns => _columns;
    //    public int CurrentRowCount { get; private set; }

    //    public Func<XArray> ColumnGetter(int columnIndex)
    //    {
    //        ColumnDetails column = Columns[columnIndex];
    //        Func<XArray>[] gettersPerSource = new Func<XArray>[_sources.Length];
    //        Array nullArray = null;
    //        Allocator.AllocateToSize(ref nullArray, 1, column.Type);

    //        // Get and cache the corresponding column getter from each source (null if the source doesn't have that column)
    //        for (int i = 0; i < _sources.Length; ++i)
    //        {
    //            int sourceColumnIndex;
    //            if (_sources[i].Columns.TryGetIndexOfColumn(column.Name, out sourceColumnIndex)) gettersPerSource[i] = _sources[i].ColumnGetter(sourceColumnIndex);
    //        }

    //        // When called, return the xarray from the active source for that column
    //        return () =>
    //        {
    //            // Find the getter for the current source
    //            Func<XArray> getter = gettersPerSource[_currentSourceIndex];

    //            // If this source didn't have the column, return all null
    //            if (getter == null) return XArray.Null(nullArray, CurrentRowCount);

    //            // Otherwise, return the current xarray
    //            return getter();
    //        };
    //    }

    //    public int Next(int desiredCount)
    //    {
    //        while (_currentSourceIndex < _sources.Length)
    //        {
    //            // Try to get rows from the next source
    //            CurrentRowCount = _sources[_currentSourceIndex].Next(desiredCount);

    //            // If it had rows left, return them
    //            if (CurrentRowCount > 0) return CurrentRowCount;

    //            // If not, try the next source
    //            _currentSourceIndex++;
    //        }

    //        // If out of sources, return no rows
    //        return 0;
    //    }

    //    public void Reset()
    //    {
    //        _currentSourceIndex = 0;

    //        foreach (IXTable source in _sources)
    //        {
    //            source.Reset();
    //        }
    //    }

    //    public void Dispose()
    //    {
    //        if (_sources != null)
    //        {
    //            foreach (IXTable source in _sources)
    //            {
    //                if (source != null) source.Dispose();
    //            }

    //            _sources = null;
    //        }
    //    }
    //}
}
