/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2022                                                               *
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

#include <QApplication>
#include <QMainWindow>
#include <QScreen>
#include <string>
#include "sgctedit/monitorbox.h"
#include "sgctedit/windowcontrol.h"
#include "sgctedit/display.h"

Display::Display(MonitorBox* monitorRenderBox, std::vector<QRect>& monitorSizeList,
                                 const unsigned int nMaxWindows, const QString* winColors)
    : _monBox(monitorRenderBox)
    , _monitorResolutions(monitorSizeList)
    , _nMaxWindows(nMaxWindows)
    , _winColors(winColors)
{
    _addWindowButton = new QPushButton("Add Window", this);
    _removeWindowButton = new QPushButton("Remove Window", this);
    _nMonitors = _monitorResolutions.size();
    //Add all window controls (some will be hidden from GUI initially)
    for (unsigned int i = 0; i < _nMaxWindows; ++i) {
        initializeWindowControl();
    }
    connect(_addWindowButton, SIGNAL(released()), this, SLOT(addWindow()));
    connect(_removeWindowButton, SIGNAL(released()), this, SLOT(removeWindow()));
    initializeLayout();
}

Display::~Display() {
    delete _monBox;
    for (auto w : _windowControl) {
        delete w;
    }
    delete _layout;
}

void Display::initializeLayout() {
    _layout = new QVBoxLayout(this);
    {
        QHBoxLayout* layoutMonButton = new QHBoxLayout();
        layoutMonButton->addWidget(_removeWindowButton);
        layoutMonButton->addStretch(1);
        layoutMonButton->addWidget(_addWindowButton);
        _layout->addLayout(layoutMonButton);
    }
    QHBoxLayout* layoutWindows = new QHBoxLayout();
    _layout->addStretch();

    for (unsigned int i = 0; i < _nMaxWindows; ++i) {
        _winCtrlLayouts.push_back(_windowControl[i]->initializeLayout());
        _layoutWindowWrappers.push_back(new QWidget());
        _layoutWindowWrappers.back()->setLayout(_winCtrlLayouts.back());
        layoutWindows->addWidget(_layoutWindowWrappers.back());
        if (i < (_nMaxWindows - 1)) {
            _frameBorderLines.push_back(new QFrame());
            _frameBorderLines.back()->setFrameShape(QFrame::VLine);
            layoutWindows->addWidget(_frameBorderLines.back());
        }
    }
    _nWindowsDisplayed = 1;
    showWindows();
    _layout->addLayout(layoutWindows);
}

std::vector<WindowControl*> Display::windowControls() {
    return _windowControl;
}

unsigned int Display::nWindows() const {
    return _nWindowsDisplayed;
}

void Display::addWindow() {
    if (_nWindowsDisplayed < _nMaxWindows) {
        _nWindowsDisplayed++;
        showWindows();
    }
}

void Display::removeWindow() {
    if (_nWindowsDisplayed > 1) {
        _nWindowsDisplayed--;
        showWindows();
    }
}

void Display::showWindows() {
    for (unsigned int i = 0; i < _layoutWindowWrappers.size(); ++i) {
        _layoutWindowWrappers[i]->setVisible(i < _nWindowsDisplayed);
    }
    for (unsigned int i = 0; i < _frameBorderLines.size(); ++i) {
        _frameBorderLines[i]->setVisible(i < (_nWindowsDisplayed - 1));
    }
    _removeWindowButton->setEnabled(_nWindowsDisplayed > 1);
    _addWindowButton->setEnabled(_nWindowsDisplayed != _nMaxWindows);
    for (auto w : _windowControl) {
        w->showWindowLabel(_nWindowsDisplayed > 1);
    }
    _monBox->setNumWindowsDisplayed(_nWindowsDisplayed);
}

void Display::initializeWindowControl() {
    if (_nWindowsAllocated < _nMaxWindows) {
        unsigned int monitorNumForThisWindow = (_nWindowsAllocated >= 3) ? 1 : 0;
        _windowControl.push_back(
            new WindowControl(
                _nMonitors,
                monitorNumForThisWindow,
                _nWindowsAllocated,
                _monitorResolutions,
                _winColors,
                this
            )
        );
        _windowControl.back()->setWindowChangeCallback(
            [this](int monIndex, int winIndex, const QRectF& newDims) {
                _monBox->windowDimensionsChanged(monIndex, winIndex, newDims);
            }
        );
        _windowControl.back()->setWebGuiChangeCallback(
            [this](unsigned int winIndex) {
                for (unsigned int w = 0; w < _nMaxWindows; ++w) {
                    if (w != winIndex) {
                        _windowControl[w]->uncheckWebGuiOption();
                    }
                }
            }
        );
        _monBox->mapWindowResolutionToWidgetCoordinates(
            monitorNumForThisWindow,
            _nWindowsAllocated,
            _windowControl.back()->dimensions()
        );
        _nWindowsAllocated++;
    }
}

void Display::uncheckWebGuiOptions() {
    for (WindowControl* w : _windowControl) {
        w->uncheckWebGuiOption();
    }
}

