/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2020                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#ifndef __OPENSPACE_UI_LAUNCHER___ASSETTREEMODEL___H__
#define __OPENSPACE_UI_LAUNCHER___ASSETTREEMODEL___H__

#include <QAbstractItemModel>
#include "assettreeitem.h"
#include "openspace/scene/profile.h"

struct importElement
{
    importElement(std::string l, int lev, bool chk)
        : line(l), level(lev), checked(chk) {}
    std::string line;
    int level = -1;
    bool checked = false;
    bool existsInFilesystem = true;
};

class assetTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit assetTreeModel(QString header1, QString header2,
        QObject* parent = nullptr);
    ~assetTreeModel();

    /**
      * Returns the data contained at an index
      *
      * \param index that defines where the item is located in the tree model
      * \param role Qt-defined role that describes the reason Qt is calling the
      *             function (can be multiple times)
      * \return QVariant data object
      */
    QVariant data(const QModelIndex &index, int role) const override;

    /**
      * Returns the header data of the tree view
      *
      * \param section of data to be obtained from header
      * \param orientation of the query (e.g. Qt::horizontal)
      * \param role Qt-defined role that describes the reason Qt is calling the
      *             function (can be multiple times)
      * \return QVariant data object in the header
      */
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    /**
      * Returns the index of item in #QModelIndex object form
      *
      * \param row the row number
      * \param column the column number
      * \param parent index of parent
      * \return #QModelIndex index of the item at specified position
      */
    QModelIndex index(int row, int column,
        const QModelIndex &parent = QModelIndex()) const override;

    /**
      * Returns the index of the parent of the item specified by input param
      *
      * \param index of item that is a child of the parent
      * \return #QModelIndex index of the parent
      */
    QModelIndex parent(const QModelIndex &index) const override;

    /**
      * Returns the index of the parent of the item specified by the input params
      *
      * \param row the row number
      * \param column the column number
      * \param parent index of parent
      * \return #QModelIndex index of the parent
      */
    QModelIndex parent(int row, int column,
        const QModelIndex& parent = QModelIndex()) const;

    /**
      * Returns asset item at specified index
      *
      * \param index of item that is a child of the parent
      * \return #assetTreeItem pointer to the item at the provided index
      */
    assetTreeItem* assetItem(const QModelIndex &index);

    /**
      * Returns number of children/rows of the parent
      *
      * \param parent #QModelIndex of the parent item
      * \return number of children/rows of this parent
      */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
      * Returns the number of columns of data in each item of the tree
      *
      * \param parent specified by the #QModelIndex index
      * \return the number of data columns
      */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
      * Return the Qt flags of the item specified by index, which can include
      * Qt::ItemIsEnabled, Qt::ItemIsSelectable
      *
      * \param index specified by the #QModelIndex index
      * \return the Qt flags
      */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /**
      * Set data at index \p index
      *
      * \param index location of the item to set
      * \param value The #QVariant data element to store at column \p column
      * \param role Qt-specific role to define context of the call
      * \return true if the data set was successful
      */
    bool setData(const QModelIndex &index, const QVariant &value,
        int role = Qt::EditRole) override;

    /**
      * Returns a vector of all #Assets selected in the tree view
      *
      * \param outputPaths vector of #openspace::Profile::Asset objects,
      *                    each of which are selected
      * \param outputItems vector of #assetTreeItem * objects,
      *                    each of which are selected
      */
    void selectedAssets(std::vector<std::string>& outputPaths,
        std::vector<assetTreeItem*>& outputItems);

    /**
      * Returns a vector of all assets selected in the tree view, but in the form of
      * #assetTreeItem pointers
      *
      * \return vector of #assetTreeItem * objects, each of which are selected
      */
    std::vector<std::string> selectedAssetsDetailed();

    /**
      * Imports asset tree data for this model. The import text format is unique to
      * this asset tree structure.
      * Each line starts with an character (0, 1, or x) to represent its checked status
      * or if it doesn't exist in the current filesystem.
      * This is followed by the name of either an asset or directory, with a space for
      * each level of sub-directory. Example:
      * 0Base
      * 0 Directory
      * 0  Asset1
      * 1  Asset2
      * 0 Asset3
      * 1Asset4
      * This format is used internally to translate from code that reads the filesystem
      *
      * \param contents asset recursive listing of directory in format described above
      */
    void importModelData(const std::string contents);

    /**
      * Returns bool for if item is checked/selected
      *
      * \param index location of the item to set
      * \return true if the item is checked
      */
    bool isChecked(QModelIndex& index) const;

    /**
      * Answers query about whether or not item is an asset
      *
      * \param index location of the item to query
      * \return true if the item is an asset (and not a directory)
      */
    bool isAsset(QModelIndex& index) const;

    /**
      * Answers query about whether or not item is in the current filesystem
      *
      * \param index location of the item to query
      * \return true if the data is in the filesystem
      */
    bool inFilesystem(QModelIndex& index) const;

    /**
      * Returns number of child items of referenced item
      *
      * \param index location of the item to query
      * \return number of child items
      */
    int childCount(QModelIndex& index) const;

    /**
      * Returns a pointer to a child item of the current item
      *
      * \param row the child number of the current item
      * \return assetTreeItem pointer to the child
      */
    assetTreeItem* child(int row) const;

    /**
      * Returns the asset name of the specified item
      *
      * \param index location of the item to query
      * \return the asset name of the item
      */
    QString name(QModelIndex& index) const;

    /**
      * Set asset name at specified index
      *
      * \param index location of the item to set
      * \param name the asset name to set
      */
    void setName(QModelIndex& index, QString name);

    /**
      * Set state of checked/selected of an item
      *
      * \param index location of the item to set
      * \param checked true if item is checked/selected
      */
    void setChecked(QModelIndex& index, bool checked);

    /**
      * Set state of whether or not asset exists in filesystem
      *
      * \param index location of the item to set
      * \param fileExists true if asset exists in filesystem
      */
    void setExistenceInFilesystem(QModelIndex& index, bool fileExists);

private:
    std::string headerTitle;
    assetTreeItem *getItem(const QModelIndex &index) const;
    assetTreeItem *rootItem;
    void parseChildrenForSelected(assetTreeItem* item,
        std::vector<std::string>& outputPaths,
        std::vector<assetTreeItem*>& outputItems, std::string pathPrefix);
    void importInsertItem(std::istringstream& iss, assetTreeItem* parent,
        importElement& elem, int level);
    bool importGetNextLine(importElement& elem, std::istringstream& iss);
    void trimWhitespaceFromLine(std::string& line);
    int getLevelFromLine(std::string line);
};

#endif // __OPENSPACE_UI_LAUNCHER___ASSETTREEMODEL___H__
