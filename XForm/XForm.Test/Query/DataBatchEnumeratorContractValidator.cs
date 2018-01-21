﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using XForm.Data;
using System.Linq;

namespace XForm.Test.Query
{
    internal class ValidatingColumn : IXColumn
    {
        private ValidatingTable _table;
        private IXColumn _column;

        public ValidatingColumn(ValidatingTable table, IXColumn column)
        {
            _table = table;
            _column = column;
        }

        public ColumnDetails ColumnDetails => _column.ColumnDetails;
        public Type IndicesType => _column.IndicesType;

        public Func<XArray> CurrentGetter()
        {
            if(_table.NextCalled) throw new AssertFailedException("Column Getters must all be requested before the first Next() call (so callees know what to retrieve).");
            return _column.CurrentGetter();
        }

        public Func<ArraySelector, XArray> SeekGetter()
        {
            if (_table.NextCalled) throw new AssertFailedException("Column Getters must all be requested before the first Next() call (so callees know what to retrieve).");
            return _column.SeekGetter();
        }

        public Func<XArray> ValuesGetter()
        {
            return _column.ValuesGetter();
        }

        public Func<XArray> IndicesCurrentGetter()
        {
            if(_table.NextCalled) throw new AssertFailedException("Column Getters must all be requested before the first Next() call (so callees know what to retrieve).");
            return _column.IndicesCurrentGetter();
        }

        public Func<ArraySelector, XArray> IndicesSeekGetter()
        {
            if(_table.NextCalled) throw new AssertFailedException("Column Getters must all be requested before the first Next() call (so callees know what to retrieve).");
            return _column.IndicesSeekGetter();
        }
    }

    public class ValidatingTable : IXTable
    {
        private IXTable _inner;
        private ValidatingColumn[] _columns;

        public bool NextCalled;
        public bool DisposeCalled;
        public int CurrentRowCount { get; private set; }

        public ValidatingTable(IXTable inner)
        {
            _inner = inner;
            _columns = inner.Columns.Select((col) => new ValidatingColumn(this, col)).ToArray();
        }

        public IReadOnlyList<IXColumn> Columns => _columns;
        public ArraySelector CurrentSelector => _inner.CurrentSelector;

        public void Dispose()
        {
            DisposeCalled = true;

            if (_inner != null)
            {
                _inner.Dispose();
                _inner = null;
            }
        }

        public int Next(int desiredCount)
        {
            NextCalled = true;

            CurrentRowCount = _inner.Next(desiredCount);
            Assert.AreEqual(CurrentRowCount, _inner.CurrentRowCount, $"Enumerator must return the same row count from Next {CurrentRowCount:n0} that it saves in CurrentRowbatchCount {_inner.CurrentRowCount:n0}.");
            return CurrentRowCount;
        }

        public void Reset()
        {
            _inner.Reset();
        }
    }
}
