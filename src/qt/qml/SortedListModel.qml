// Implements a convenient ListModel that contains sorted items.
//
// properties:
//   sortKey: specifies one or more key to sort on with an optional "-" prefix to set descending order
//   compareFunc: allows you to replace the default compare function with your own
//
// methods:
//   insertSorted(obj) - uses binary search insertion sort, duplicates aren't allowed, last in wins.
//   sort() - leverages from Array.prototype.sort() to rapidly sort all items to a new compareFunc
//   load() - load from another model

import QtQuick 2.9

ListModel {
    id: sortedListModel

    property string sortKey: "value"
    property var options: getOptions(sortKey)
    property var compareFunc: null

    onCompareFuncChanged: sort()
    onSortKeyChanged: sort()
    onOptionsChanged: sort()

    function naturalPad(str) { return ("0000000" + str).substr(-8); }
    function natural(str) { return str.replace(/\d+/g, naturalPad); }
    function stringCompareNatural(str1, str2) { return natural(str1).localeCompare(natural(str2)); }
    function stringCompare(str1, str2) { return stringCompareNatural(str1, str2); }

    function compareValues(val1, val2) {
        if (val1 === undefined && val2 === undefined) return 0;
        if (typeof(val1) === typeof(val2)) {
            switch (typeof(val1)) {
            case "string": return stringCompare(val1, val2);
            case "number": return val1 - val2;
            }
        }
        return compareValues(val1.toString(), val2.toString());
    }

    function defaultCompareFunc(obj1, obj2) {
        for (var i = 0; i < options.length; i++) {
            var option = options[i];
            var cmp = compareValues(obj1[option.key], obj2[option.key]) * option.ascending;
            if (cmp !== 0) return cmp;
        }
        return 0;
    }

    function internalCompareFunc(obj1, obj2) {
        return compareFunc !== null ? compareFunc(obj1, obj2) : defaultCompareFunc(obj1, obj2);
    }

    function sort() { listModelSort(sortedListModel, internalCompareFunc); }
    function insertSorted(item, options) { listModelInsertSorted(sortedListModel, internalCompareFunc, item, options); }

    function getOption(optionDef) {
        var m = optionDef.match(/^-(.*)$/);
        return m ?  { ascending: -1, key: m[1] } :  { ascending: 1, key: optionDef };
    }

    function getOptions(sortKey) { return sortKey.split(/[ ,]/).map(getOption); }

    function load(model) {
        clear();
        if (model === null) return;
        insertSorted(model);
    }

    function listModelSort(listModel, compareFunc) {
        var indexes = new Array(listModel.count);
        for (var i = 0; i < listModel.count; i++) indexes[i] = i;
        indexes.sort(function (indexA, indexB) { return compareFunc(get(indexA), get(indexB)) } );
        var sorted = 0;
        while (sorted < indexes.length && sorted === indexes[sorted]) sorted++;
        if (sorted === indexes.length) return;
        for (i = sorted; i < indexes.length; i++) {
            var idx = indexes[i];
            listModel.move(idx, listModel.count - 1, 1);
            listModel.insert(idx, { } );
        }
        listModel.remove(sorted, indexes.length - sorted);
    }

    function listModelIndexOf(listModel, startIndex, endIndex, item, compareFunc) {
        if (startIndex > endIndex) return startIndex;
        var startItem = listModel.get(startIndex);
        var startCmp = compareFunc(item, startItem);
        if (startCmp <= 0) return startIndex;
        if (endIndex <= startIndex) return startIndex + 1;
        var endItem = listModel.get(endIndex);
        var endCmp = compareFunc(item, endItem);
        if (endCmp === 0) return endIndex;
        if (endCmp > 0) return endIndex + 1;
        while (endIndex > startIndex + 1) {
            var midIndex = (startIndex + endIndex) >> 1;
            var midItem = listModel.get(midIndex);
            var midCmp = compareFunc(item, midItem);
            if (midCmp === 0) return midIndex;
            if (midCmp > 0)
                endIndex = midIndex;
            else
                startIndex = midIndex;
        }
        return endIndex;
    }

    function listModelInsertSorted(listModel, compareFunc, item, options) {
        if (Array.isArray(item)) {
            listModelInsertSortedArray(listModel, compareFunc, item, options);
            return;
        }

        try {
            if (item instanceof ListModel) {
                listModelInsertedSortedListModel(item, options);
                return;
            }
        } catch (err) {
        }

        if (listModel.count === 0) {
            listModel.append(item);
            return;
        }
        var index = listModelIndexOf(listModel, 0, listModel.count - 1, item, compareFunc);
        if (index >= listModel.count) {
            listModel.append(item);
            return;
        }
        var cmp = compareFunc(item, listModel.get(index));
        if (cmp === 0) {
            if (options && options.unique) {
                listModel.set(index, item);
                return;
            }
        }
        listModel.insert(index, item);
    }

    function listModelInsertSortedArray(listModel, compareFunc, arr, options) {
        for (var i = 0; i < arr.length; i++)
            listModelInsertSorted(listModel, compareFunc, arr[i], options);
    }

    function listModelInsertSortedListModel(listModel, compareFunc, model, options) {
        for (var i = 0; i < model.count; i++)
            listModelInsertSorted(listModel, compareFunc, model.get(i), options);
    }
}
