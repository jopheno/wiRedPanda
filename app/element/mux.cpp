// Copyright 2015 - 2022, GIBIS-Unifesp and the WiRedPanda contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mux.h"

#include "common.h"
#include "qneport.h"

namespace
{
int id = qRegisterMetaType<Mux>();
}

Mux::Mux(QGraphicsItem *parent)
    : GraphicElement(ElementType::Mux, ElementGroup::Mux, 3, 3, 1, 1, parent)
{
    qCDebug(zero) << "Creating mux.";

    m_pixmapSkinName = QStringList{":/basic/mux.png"};
    setPixmap(m_pixmapSkinName.first());

    setRotatable(true);
    Mux::updatePorts();
    setPortName("MUX");
    setToolTip(m_translatedName);
    setCanChangeSkin(true);

    input(0)->setName("0");
    input(1)->setName("1");
    input(2)->setName("S");
}

void Mux::updatePorts()
{
    input(0)->setPos(32 - 12, 48); /* 0   */
    input(1)->setPos(32 + 12, 48); /* 1   */
    input(2)->setPos(58, 32);      /* S   */

    output(0)->setPos(32, 16);     /* Out */
}

void Mux::setSkin(const bool defaultSkin, const QString &fileName)
{
    m_pixmapSkinName[0] = (defaultSkin) ? ":/basic/mux.png" : fileName;
    setPixmap(m_pixmapSkinName[0]);
}
