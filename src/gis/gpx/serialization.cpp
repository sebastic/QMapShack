/**********************************************************************************************
    Copyright (C) 2014 Oliver Eichler oliver.eichler@gmx.de

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/

#include "gis/IGisProject.h"
#include "gis/wpt/CGisItemWpt.h"
#include "gis/trk/CGisItemTrk.h"
#include "gis/rte/CGisItemRte.h"
#include "gis/ovl/CGisItemOvlArea.h"
#include "gis/WptIcons.h"
#include "version.h"

#include <QtXml>
#include <proj_api.h>
const QString IGisProject::gpx_ns      = "http://www.topografix.com/GPX/1/1";
const QString IGisProject::xsi_ns      = "http://www.w3.org/2001/XMLSchema-instance";
const QString IGisProject::gpxx_ns     = "http://www.garmin.com/xmlschemas/GpxExtensions/v3";
const QString IGisProject::gpxtpx_ns   = "http://www.garmin.com/xmlschemas/TrackPointExtension/v1";
const QString IGisProject::wptx1_ns    = "http://www.garmin.com/xmlschemas/WaypointExtension/v1";
const QString IGisProject::rmc_ns      = "urn:net:trekbuddy:1.0:nmea:rmc";
const QString IGisProject::ql_ns       = "http://www.qlandkarte.org/xmlschemas/v1.1";
const QString IGisProject::gs_ns       = "http://www.groundspeak.com/cache/1/0";

static void readXml(const QDomNode& xml, const QString& tag, bool& value)
{
    if(xml.namedItem(tag).isElement())
    {
        bool tmp;
        bool ok = false;
        tmp = xml.namedItem(tag).toElement().text().toInt(&ok);
        if(ok)
        {
            value = tmp;
        }
    }
}

static void readXml(const QDomNode& xml, const QString& tag, qint32& value)
{
    if(xml.namedItem(tag).isElement())
    {
        qint32 tmp;
        bool ok = false;
        tmp = xml.namedItem(tag).toElement().text().toInt(&ok);
        if(!ok)
        {
            tmp = qRound(xml.namedItem(tag).toElement().text().toDouble(&ok));
        }
        if(ok)
        {
            value = tmp;
        }
    }
}

static void readXml(const QDomNode& xml, const QString& tag, quint32& value)
{
    if(xml.namedItem(tag).isElement())
    {
        quint32 tmp;
        bool ok = false;
        tmp = xml.namedItem(tag).toElement().text().toUInt(&ok);
        if(ok)
        {
            value = tmp;
        }
    }
}

static void readXml(const QDomNode& xml, const QString& tag, quint64& value)
{
    if(xml.namedItem(tag).isElement())
    {
        quint64 tmp;
        bool ok = false;
        tmp = xml.namedItem(tag).toElement().text().toULongLong(&ok);
        if(ok)
        {
            value = tmp;
        }
    }
}

static void readXml(const QDomNode& xml, const QString& tag, qreal& value)
{
    if(xml.namedItem(tag).isElement())
    {
        qreal tmp;
        bool ok = false;
        tmp = xml.namedItem(tag).toElement().text().toDouble(&ok);
        if(ok)
        {
            value = tmp;
        }
    }
}

static void readXml(const QDomNode& xml, const QString& tag, QString& value)
{
    if(xml.namedItem(tag).isElement())
    {
        value = xml.namedItem(tag).toElement().text();
    }
}

static void readXml(const QDomNode& xml, const QString& tag, QString& value, bool& isHtml)
{
    if(xml.namedItem(tag).isElement())
    {
        const QDomNamedNodeMap& attr = xml.namedItem(tag).toElement().attributes();

        if(attr.namedItem("html").nodeValue().toLocal8Bit().toLower() == "true")
        {
            isHtml = true;
            value = xml.namedItem(tag).toElement().text();
        }
        else
        {
            isHtml = false;
            value = "<pre>" + xml.namedItem(tag).toElement().text() + "</pre>";
            value = xml.namedItem(tag).toElement().text();
        }

    }
}

static void readXml(const QDomNode& xml, const QString& tag, QDateTime& value)
{
    if(xml.namedItem(tag).isElement())
    {
        QString time = xml.namedItem(tag).toElement().text();
        IUnit::parseTimestamp(time, value);

    }
}

static void readXml(const QDomNode& xml, const QString& tag, QList<IGisItem::link_t>& l)
{
    if(xml.namedItem(tag).isElement())
    {
        const QDomNodeList& links = xml.toElement().elementsByTagName(tag);
        int N = links.count();
        for(int n = 0; n < N; ++n)
        {
            const QDomNode& link = links.item(n);

            IGisItem::link_t tmp;
            tmp.uri.setUrl(link.attributes().namedItem("href").nodeValue());
            readXml(link, "text", tmp.text);
            readXml(link, "type", tmp.type);

            l << tmp;
        }
    }
}

static void readXml(const QDomNode& xml, IGisItem::history_t& history)
{
    if(xml.namedItem("ql:history").isElement())
    {
        const QDomElement& xmlHistory = xml.namedItem("ql:history").toElement();

        const QDomNodeList& xmlEntries = xmlHistory.elementsByTagName("ql:event");
        int N = xmlEntries.count();
        for(int n = 0; n < N; ++n)
        {
            const QDomNode& xmlEntry = xmlEntries.item(n);
            IGisItem::history_event_t entry;
            readXml(xmlEntry, "ql:icon", entry.icon);
            readXml(xmlEntry, "ql:time", entry.time);
            readXml(xmlEntry, "ql:comment", entry.comment);

            history.events << entry;
        }

        history.histIdxInitial = history.events.size() - 1;
        history.histIdxCurrent = history.histIdxInitial;
    }
}


static void writeXml(QDomNode& xml, const QString& tag, qint32 val)
{
    if(val != NOINT)
    {
        QDomElement elem = xml.ownerDocument().createElement(tag);
        xml.appendChild(elem);
        QDomText text = xml.ownerDocument().createTextNode(QString::number(val));
        elem.appendChild(text);
    }
}

static void writeXml(QDomNode& xml, const QString& tag, quint32 val)
{
    if(val != NOINT)
    {
        QDomElement elem = xml.ownerDocument().createElement(tag);
        xml.appendChild(elem);
        QDomText text = xml.ownerDocument().createTextNode(QString::number(val));
        elem.appendChild(text);
    }
}

static void writeXml(QDomNode& xml, const QString& tag, quint64 val)
{
    if(val != 0)
    {
        QDomElement elem = xml.ownerDocument().createElement(tag);
        xml.appendChild(elem);
        QDomText text = xml.ownerDocument().createTextNode(QString::number(val));
        elem.appendChild(text);
    }
}

static void writeXml(QDomNode& xml, const QString& tag, const QString& val)
{
    if(!val.isEmpty())
    {
        QDomElement elem = xml.ownerDocument().createElement(tag);
        xml.appendChild(elem);
        QDomText text = xml.ownerDocument().createTextNode(val);
        elem.appendChild(text);
    }
}

static void writeXml(QDomNode& xml, const QString& tag, qreal val)
{
    if(val != NOFLOAT)
    {
        QDomElement elem = xml.ownerDocument().createElement(tag);
        xml.appendChild(elem);
        QDomText text = xml.ownerDocument().createTextNode(QString("%1").arg(val,0,'f',8));
        elem.appendChild(text);
    }
}

static void writeXml(QDomNode& xml, const QString& tag, const QString& val, bool isHtml)
{
    if(!val.isEmpty())
    {
        QDomElement elem = xml.ownerDocument().createElement(tag);
        xml.appendChild(elem);
        QDomText text = xml.ownerDocument().createCDATASection(val);
        elem.appendChild(text);
        elem.setAttribute("html",isHtml ? "True" : "False");

    }
}

static void writeXml(QDomNode& xml, const QString& tag, const QDateTime& time)
{
    if(time.isValid())
    {
        QDomElement elem = xml.ownerDocument().createElement(tag);
        xml.appendChild(elem);
        QDomText text = xml.ownerDocument().createTextNode(time.toString("yyyy-MM-dd'T'hh:mm:ss'Z'"));
        elem.appendChild(text);
    }
}


static void writeXml(QDomNode& xml, const QString& tag, const QList<IGisItem::link_t>& links)
{
    if(!links.isEmpty())
    {
        foreach(const IGisItem::link_t& link, links)
        {
            QDomElement elem = xml.ownerDocument().createElement(tag);
            xml.appendChild(elem);

            elem.setAttribute("href", link.uri.toString());
            writeXml(elem, "text", link.text);
            writeXml(elem, "type", link.type);
        }
    }
}

static void writeXml(QDomNode& xml, const IGisItem::history_t& history)
{
    if(history.events.size() > 1)
    {
        QDomElement xmlHistory = xml.ownerDocument().createElement("ql:history");
        xml.appendChild(xmlHistory);
        for(int i = 0; i <= history.histIdxCurrent; i++)
        {
            const IGisItem::history_event_t& event = history.events[i];
            QDomElement xmlEvent = xml.ownerDocument().createElement("ql:event");
            xmlHistory.appendChild(xmlEvent);
            writeXml(xmlEvent,"ql:icon", event.icon);
            writeXml(xmlEvent,"ql:time", event.time);
            writeXml(xmlEvent,"ql:comment", event.comment);

        }
    }
}


void IGisProject::readMetadata(const QDomNode& xml, metadata_t& metadata)
{

    readXml(xml,"name", metadata.name);
    readXml(xml,"desc", metadata.desc);

    const QDomNode& xmlAuthor = xml.namedItem("author");
    if(xmlAuthor.isElement())
    {
        readXml(xml,"name", metadata.author.name);

        const QDomNode& xmlEmail = xmlAuthor.namedItem("email");
        if(xmlEmail.isElement())
        {
            const QDomNamedNodeMap& attr = xmlEmail.attributes();
            metadata.author.id = attr.namedItem("id").nodeValue();
            metadata.author.domain = attr.namedItem("domain").nodeValue();
        }

        const QDomNode& xmlLink = xmlAuthor.namedItem("link");
        if(xmlLink.isElement())
        {
            metadata.author.link.uri.setUrl(xmlLink.attributes().namedItem("href").nodeValue());
            readXml(xmlLink, "text", metadata.author.link.text);
            readXml(xmlLink, "type", metadata.author.link.type);
        }
    }

    const QDomNode& xmlCopyright = xml.namedItem("copyright");
    if(xmlCopyright.isElement())
    {
        metadata.copyright.author = xmlCopyright.attributes().namedItem("author").nodeValue();
        readXml(xmlCopyright, "year", metadata.copyright.year);
        readXml(xmlCopyright, "license", metadata.copyright.license);
    }

    readXml(xml,"link", metadata.links);
    readXml(xml,"time", metadata.time);
    readXml(xml,"keywords", metadata.keywords);

    const QDomNode& xmlBounds = xml.namedItem("bounds");
    if(xmlBounds.isElement())
    {
        const QDomNamedNodeMap& attr = xmlBounds.attributes();
        metadata.bounds.setLeft(attr.namedItem("minlon").nodeValue().toDouble());
        metadata.bounds.setTop(attr.namedItem("maxlat").nodeValue().toDouble());
        metadata.bounds.setRight(attr.namedItem("maxlon").nodeValue().toDouble());
        metadata.bounds.setBottom(attr.namedItem("minlat").nodeValue().toDouble());
    }
}

QDomNode IGisProject::writeMetadata(QDomDocument& doc)
{
    QDomElement gpx = doc.createElement("gpx");
    doc.appendChild(gpx);

    gpx.setAttribute("version","1.1");
    gpx.setAttribute("creator","QMapShack " VER_STR " http://www.qlandkarte.org/");
    gpx.setAttribute("xmlns",gpx_ns);
    gpx.setAttribute("xmlns:xsi",xsi_ns);
    gpx.setAttribute("xmlns:gpxx",gpxx_ns);
    gpx.setAttribute("xmlns:gpxtpx",gpxtpx_ns);
    gpx.setAttribute("xmlns:wptx1",wptx1_ns);
    gpx.setAttribute("xmlns:rmc",rmc_ns);
    gpx.setAttribute("xmlns:ql",ql_ns);

    QString schemaLocation = QString()
        + gpx_ns    + " http://www.topografix.com/GPX/1/1/gpx.xsd "
        + gpxx_ns   + " http://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd "
        + gpxtpx_ns + " http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd "
        + wptx1_ns  + " http://www.garmin.com/xmlschemas/WaypointExtensionv1.xsd"
        + ql_ns     + " http://www.qlandkarte.org/xmlschemas/v1.1/ql-extensions.xsd";

    gpx.setAttribute("xsi:schemaLocation", schemaLocation);

    QDomElement xmlMetadata = doc.createElement("metadata");
    gpx.appendChild(xmlMetadata);

    writeXml(xmlMetadata,"name", metadata.name);
    writeXml(xmlMetadata,"desc", metadata.desc);

    if(!metadata.author.name.isEmpty())
    {
        QDomElement xmlAuthor = doc.createElement("author");
        xmlMetadata.appendChild(xmlAuthor);

        writeXml(xmlAuthor,"name", metadata.author.name);

        if(!metadata.author.id.isEmpty() && !metadata.author.domain.isEmpty())
        {
            QDomElement xmlEmail = doc.createElement("email");
            xmlAuthor.appendChild(xmlEmail);
            xmlEmail.setAttribute("id", metadata.author.id);
            xmlEmail.setAttribute("domain", metadata.author.domain);
        }

        if(metadata.author.link.uri.isValid())
        {
            QDomElement xmlLink = doc.createElement("link");
            xmlAuthor.appendChild(xmlLink);

            xmlLink.setAttribute("href", metadata.author.link.uri.toString());
            writeXml(xmlLink, "text", metadata.author.link.text);
            writeXml(xmlLink, "type", metadata.author.link.type);

        }
    }

    if(!metadata.copyright.author.isEmpty())
    {
        QDomElement xmlCopyright = doc.createElement("copyright");
        xmlMetadata.appendChild(xmlCopyright);

        xmlCopyright.setAttribute("author", metadata.copyright.author);
        writeXml(xmlCopyright, "year", metadata.copyright.year);
        writeXml(xmlCopyright, "license", metadata.copyright.license);

    }
    writeXml(xmlMetadata, "link", metadata.links);
    writeXml(xmlMetadata, "time", metadata.time);
    writeXml(xmlMetadata, "keywords", metadata.keywords);

    if(metadata.bounds.isValid())
    {
        QDomElement xmlBounds = doc.createElement("bounds");
        xmlMetadata.appendChild(xmlBounds);

        xmlBounds.setAttribute("minlat", metadata.bounds.bottom());
        xmlBounds.setAttribute("minlon", metadata.bounds.left());
        xmlBounds.setAttribute("maxlat", metadata.bounds.top());
        xmlBounds.setAttribute("maxlon", metadata.bounds.right());

    }

    return gpx;
}

void CGisItemWpt::readGpx(const QDomNode& xml)
{
    readWpt(xml, wpt);
    // decode some well known extensions
    if(xml.namedItem("extensions").isElement())
    {
        const QDomNode& ext = xml.namedItem("extensions");
        readXml(ext, "ql:key", key);
        readXml(ext, "ql:flags", flags);
        readXml(ext, history);

        const QDomNode& wptx1 = ext.namedItem("wptx1:WaypointExtension");
        readXml(wptx1, "wptx1:Proximity", proximity);

        const QDomNode& xmlCache = ext.namedItem("cache");
        if(!xmlCache.isNull())
        {
            // read OC cache extensions
        }
    }

    const QDomNode& xmlCache = xml.namedItem("groundspeak:cache");
    if(!xmlCache.isNull() && !geocache.hasData)
    {
        readGcExt(xmlCache);
    }
}

void CGisItemWpt::save(QDomNode& gpx)
{
    QDomDocument doc = gpx.ownerDocument();

    QDomElement xmlWpt = doc.createElement("wpt");
    gpx.appendChild(xmlWpt);
    writeWpt(xmlWpt, wpt);

    // write the key as extension tag
    QDomElement xmlExt  = doc.createElement("extensions");
    xmlWpt.appendChild(xmlExt);
    writeXml(xmlExt, "ql:key", key);
    writeXml(xmlExt, "ql:flags", flags);
    writeXml(xmlExt, history);

    // write other well known extensions
    QDomElement wptx1  = doc.createElement("wptx1:WaypointExtension");
    xmlExt.appendChild(wptx1);
    writeXml(wptx1, "wptx1:Proximity", proximity);

    if(geocache.hasData && geocache.service == eGC)
    {
        QDomElement xmlCache = doc.createElement("groundspeak:cache");
        writeGcExt(xmlCache);
        xmlWpt.appendChild(xmlCache);
    }
}

void CGisItemWpt::readGcExt(const QDomNode& xmlCache)
{
    geocache.service = eGC;
    const QDomNamedNodeMap& attr = xmlCache.attributes();
    geocache.id = attr.namedItem("id").nodeValue().toInt();

    geocache.archived   = attr.namedItem("archived").nodeValue().toLocal8Bit() == "True";
    geocache.available  = attr.namedItem("available").nodeValue().toLocal8Bit() == "True";
    if(geocache.archived)
    {
        geocache.status = QObject::tr("Archived");
    }
    else if(geocache.available)
    {
        geocache.status = QObject::tr("Available");
    }
    else
    {
        geocache.status = QObject::tr("Not Available");
    }

    readXml(xmlCache, "groundspeak:name", geocache.name);
    readXml(xmlCache, "groundspeak:placed_by", geocache.owner);
    readXml(xmlCache, "groundspeak:type", geocache.type);
    readXml(xmlCache, "groundspeak:container", geocache.container);
    readXml(xmlCache, "groundspeak:difficulty", geocache.difficulty);
    readXml(xmlCache, "groundspeak:terrain", geocache.terrain);
    readXml(xmlCache, "groundspeak:short_description", geocache.shortDesc, geocache.shortDescIsHtml);
    readXml(xmlCache, "groundspeak:long_description", geocache.longDesc, geocache.longDescIsHtml);
    readXml(xmlCache, "groundspeak:encoded_hints", geocache.hint);
    readXml(xmlCache, "groundspeak:country", geocache.country);
    readXml(xmlCache, "groundspeak:state", geocache.state);

    const QDomNodeList& logs = xmlCache.toElement().elementsByTagName("groundspeak:log");
    uint N = logs.count();

    for(uint n = 0; n < N; ++n)
    {
        const QDomNode& xmlLog = logs.item(n);
        const QDomNamedNodeMap& attr = xmlLog.attributes();

        geocachelog_t log;
        log.id = attr.namedItem("id").nodeValue().toUInt();
        readXml(xmlLog, "groundspeak:date", log.date);
        readXml(xmlLog, "groundspeak:type", log.type);
        if(xmlLog.namedItem("groundspeak:finder").isElement())
        {
            const QDomNamedNodeMap& attr = xmlLog.namedItem("groundspeak:finder").attributes();
            log.finderId = attr.namedItem("id").nodeValue();
        }

        readXml(xmlLog, "groundspeak:finder", log.finder);
        readXml(xmlLog, "groundspeak:text", log.text, log.textIsHtml);

        geocache.logs << log;

    }
    geocache.hasData = true;
}

void CGisItemWpt::writeGcExt(QDomNode& xmlCache)
{
    QString str;
    xmlCache.toElement().setAttribute("xmlns:groundspeak", "http://www.groundspeak.com/cache/1/0");
    xmlCache.toElement().setAttribute("id", geocache.id);
    xmlCache.toElement().setAttribute("archived", geocache.archived ? "True" : "False");
    xmlCache.toElement().setAttribute("available", geocache.available ? "True" : "False");

    writeXml(xmlCache, "groundspeak:name", geocache.name);
    writeXml(xmlCache, "groundspeak:placed_by", geocache.owner);
    writeXml(xmlCache, "groundspeak:type", geocache.type);
    writeXml(xmlCache, "groundspeak:container", geocache.container);

    if(geocache.difficulty == int(geocache.difficulty))
    {
        str.sprintf("%1.0f", geocache.difficulty);
    }
    else
    {
        str.sprintf("%1.1f", geocache.difficulty);
    }
    writeXml(xmlCache, "groundspeak:difficulty", str);

    if(geocache.terrain == int(geocache.terrain))
    {
        str.sprintf("%1.0f", geocache.terrain);
    }
    else
    {
        str.sprintf("%1.1f", geocache.terrain);
    }
    writeXml(xmlCache, "groundspeak:terrain", str);
    writeXml(xmlCache, "groundspeak:short_description", geocache.shortDesc, geocache.shortDescIsHtml);
    writeXml(xmlCache, "groundspeak:long_description", geocache.longDesc, geocache.longDescIsHtml);
    writeXml(xmlCache, "groundspeak:encoded_hints", geocache.hint);

    if(!geocache.logs.isEmpty())
    {
        QDomElement xmlLogs = xmlCache.ownerDocument().createElement("groundspeak:logs");
        xmlCache.appendChild(xmlLogs);

        foreach(const geocachelog_t& log, geocache.logs)
        {
            QDomElement xmlLog = xmlCache.ownerDocument().createElement("groundspeak:log");
            xmlLogs.appendChild(xmlLog);

            xmlLog.setAttribute("id", log.id);
            writeXml(xmlLog, "groundspeak:date", log.date);
            writeXml(xmlLog, "groundspeak:type", log.type);

            QDomElement xmlFinder = xmlCache.ownerDocument().createElement("groundspeak:finder");
            xmlLog.appendChild(xmlFinder);

            QDomText _finder_ = xmlCache.ownerDocument().createCDATASection(log.finder);
            xmlFinder.appendChild(_finder_);
            xmlFinder.setAttribute("id", log.finderId);

            writeXml(xmlLog, "groundspeak:text", log.text, log.textIsHtml);
        }
    }
}


void CGisItemTrk::readTrk(const QDomNode& xml, trk_t& trk)
{
    readXml(xml, "name", trk.name);
    readXml(xml, "cmt", trk.cmt);
    readXml(xml, "desc", trk.desc);
    readXml(xml, "src", trk.src);
    readXml(xml, "link", trk.links);
    readXml(xml, "number", trk.number);
    readXml(xml, "type", trk.type);

    const QDomNodeList& trksegs = xml.toElement().elementsByTagName("trkseg");
    int N = trksegs.count();
    trk.segs.resize(N);
    for(int n = 0; n < N; ++n)
    {
        const QDomNode& trkseg = trksegs.item(n);
        trkseg_t& seg = trk.segs[n];

        const QDomNodeList& xmlTrkpts = trkseg.toElement().elementsByTagName("trkpt");
        int M = xmlTrkpts.count();
        seg.pts.resize(M);
        for(int m = 0; m < M; ++m)
        {
            trkpt_t& trkpt = seg.pts[m];
            const QDomNode& xmlTrkpt = xmlTrkpts.item(m);
            readWpt(xmlTrkpt, trkpt);

            const QDomNode& ext = xmlTrkpt.namedItem("extensions");
            if(ext.isElement())
            {
                readXml(ext, "ql:flags", trkpt.flags);
            }
        }
    }

    // decode some well known extensions
    const QDomNode& ext = xml.namedItem("extensions");
    if(ext.isElement())
    {
        readXml(ext, "ql:key", key);
        readXml(ext, "ql:flags", flags);
        readXml(ext, history);

        const QDomNode& gpxx = ext.namedItem("gpxx:TrackExtension");
        readXml(gpxx, "gpxx:DisplayColor", trk.color);
        setColor(str2color(trk.color));
    }

    deriveSecondaryData();
}

void CGisItemTrk::save(QDomNode& gpx)
{

    QDomDocument doc = gpx.ownerDocument();

    QDomElement xmlTrk = doc.createElement("trk");
    gpx.appendChild(xmlTrk);

    writeXml(xmlTrk, "name", trk.name);
    writeXml(xmlTrk, "cmt", trk.cmt);
    writeXml(xmlTrk, "desc", trk.desc);
    writeXml(xmlTrk, "src", trk.src);
    writeXml(xmlTrk, "link", trk.links);
    writeXml(xmlTrk, "number", trk.number);
    writeXml(xmlTrk, "type", trk.type);

    // write the key as extension tag
    QDomElement xmlExt  = doc.createElement("extensions");
    xmlTrk.appendChild(xmlExt);
    writeXml(xmlExt, "ql:key", key);
    writeXml(xmlExt, "ql:flags", flags);
    writeXml(xmlExt, history);

    // write other well known extensions
    QDomElement gpxx  = doc.createElement("gpxx:TrackExtension");
    xmlExt.appendChild(gpxx);
    writeXml(gpxx, "gpxx:DisplayColor", trk.color);

    foreach(const trkseg_t& seg, trk.segs)
    {
        QDomElement xmlTrkseg = doc.createElement("trkseg");
        xmlTrk.appendChild(xmlTrkseg);

        foreach(const trkpt_t& pt, seg.pts)
        {
            QDomElement xmlTrkpt = doc.createElement("trkpt");
            xmlTrkseg.appendChild(xmlTrkpt);
            writeWpt(xmlTrkpt, pt);

            QDomElement xmlExt  = doc.createElement("extensions");
            xmlTrkpt.appendChild(xmlExt);
            writeXml(xmlExt, "ql:flags", pt.flags);
        }
    }
}

void CGisItemRte::readRte(const QDomNode& xml, rte_t& rte)
{
    qreal north = -90;
    qreal east  = -180;
    qreal south =  90;
    qreal west  =  180;

    readXml(xml, "name", rte.name);
    readXml(xml, "cmt", rte.cmt);
    readXml(xml, "desc", rte.desc);
    readXml(xml, "src", rte.src);
    readXml(xml, "link", rte.links);
    readXml(xml, "number", rte.number);
    readXml(xml, "type", rte.type);

    const QDomNodeList& xmlRtepts = xml.toElement().elementsByTagName("rtept");
    int M = xmlRtepts.count();
    rte.pts.resize(M);
    for(int m = 0; m < M; ++m)
    {
        rtept_t& rtept = rte.pts[m];
        const QDomNode& xmlRtept = xmlRtepts.item(m);
        readWpt(xmlRtept, rtept);
        rtept.icon = getWptIconByName(rtept.sym, rtept.focus);

        if(rtept.lon < west)  west    = rtept.lon;
        if(rtept.lon > east)  east    = rtept.lon;
        if(rtept.lat < south) south   = rtept.lat;
        if(rtept.lat > north) north   = rtept.lat;

    }

    boundingRect = QRectF(QPointF(west * DEG_TO_RAD, north * DEG_TO_RAD), QPointF(east * DEG_TO_RAD,south * DEG_TO_RAD));

    // decode some well known extensions
    if(xml.namedItem("extensions").isElement())
    {
        const QDomNode& ext = xml.namedItem("extensions");
        readXml(ext, "ql:key", key);
    }
}


void CGisItemRte::save(QDomNode& gpx)
{
    QDomDocument doc = gpx.ownerDocument();

    QDomElement xmlRte = doc.createElement("rte");
    gpx.appendChild(xmlRte);

    writeXml(xmlRte, "name", rte.name);
    writeXml(xmlRte, "cmt", rte.cmt);
    writeXml(xmlRte, "desc", rte.desc);
    writeXml(xmlRte, "src", rte.src);
    writeXml(xmlRte, "link", rte.links);
    writeXml(xmlRte, "number", rte.number);
    writeXml(xmlRte, "type", rte.type);

    // write the key as extension tag
    QDomElement xmlExt  = doc.createElement("extensions");
    xmlRte.appendChild(xmlExt);
    writeXml(xmlExt, "ql:key", key);

    foreach(const rtept_t& pt, rte.pts)
    {
        QDomElement xmlRtept = doc.createElement("rtept");
        xmlRte.appendChild(xmlRtept);
        writeWpt(xmlRtept, pt);
    }

}

void CGisItemOvlArea::readArea(const QDomNode& xml, area_t& area)
{
    readXml(xml, "ql:name", area.name);
    readXml(xml, "ql:cmt", area.cmt);
    readXml(xml, "ql:desc", area.desc);
    readXml(xml, "ql:src", area.src);
    readXml(xml, "ql:link", area.links);
    readXml(xml, "ql:number", area.number);
    readXml(xml, "ql:type", area.type);
    readXml(xml, "ql:color", area.color);
    readXml(xml, "ql:width", area.width);
    readXml(xml, "ql:style", area.style);
    readXml(xml, "ql:opacity", area.opacity);
    readXml(xml, "ql:key", key);
    readXml(xml, "ql:flags", flags);
    readXml(xml, history);



    const QDomNodeList& xmlPts = xml.toElement().elementsByTagName("ql:point");
    int M = xmlPts.count();
    area.pts.resize(M);
    for(int m = 0; m < M; ++m)
    {
        pt_t& pt = area.pts[m];
        const QDomNode& xmlPt = xmlPts.item(m);
        readWpt(xmlPt, pt);
    }


    setColor(str2color(area.color));

    deriveSecondaryData();
}

void CGisItemOvlArea::save(QDomNode& gpx)
{
    QDomDocument doc = gpx.ownerDocument();

    QDomElement xmlArea = doc.createElement("ql:area");
    gpx.appendChild(xmlArea);

    writeXml(xmlArea, "ql:name", area.name);
    writeXml(xmlArea, "ql:cmt", area.cmt);
    writeXml(xmlArea, "ql:desc", area.desc);
    writeXml(xmlArea, "ql:src", area.src);
    writeXml(xmlArea, "ql:link", area.links);
    writeXml(xmlArea, "ql:number", area.number);
    writeXml(xmlArea, "ql:type", area.type);
    writeXml(xmlArea, "ql:color", area.color);
    writeXml(xmlArea, "ql:width", area.width);
    writeXml(xmlArea, "ql:style", area.style);
    writeXml(xmlArea, "ql:opacity", area.opacity);
    writeXml(xmlArea, "ql:key", key);
    writeXml(xmlArea, "ql:flags", flags);
    writeXml(xmlArea, history);


    foreach(const pt_t& pt, area.pts)
    {
        QDomElement xmlPt = doc.createElement("ql:point");
        xmlArea.appendChild(xmlPt);
        writeWpt(xmlPt, pt);
    }
}

void IGisItem::readWpt(const QDomNode& xml, wpt_t& wpt)
{
    const QDomNamedNodeMap& attr = xml.attributes();
    wpt.lat = attr.namedItem("lat").nodeValue().toDouble();
    wpt.lon = attr.namedItem("lon").nodeValue().toDouble();

    readXml(xml, "ele", wpt.ele);
    readXml(xml, "time", wpt.time);
    readXml(xml, "magvar", wpt.magvar);
    readXml(xml, "geoidheight", wpt.geoidheight);
    readXml(xml, "name", wpt.name);
    readXml(xml, "cmt", wpt.cmt);
    readXml(xml, "desc", wpt.desc);
    readXml(xml, "src", wpt.src);
    readXml(xml, "link", wpt.links);
    readXml(xml, "sym", wpt.sym);
    readXml(xml, "type", wpt.type);
    readXml(xml, "fix", wpt.fix);
    readXml(xml, "sat", wpt.sat);
    readXml(xml, "hdop", wpt.hdop);
    readXml(xml, "vdop", wpt.vdop);
    readXml(xml, "pdop", wpt.pdop);
    readXml(xml, "ageofdgpsdata", wpt.ageofdgpsdata);
    readXml(xml, "dgpsid", wpt.dgpsid);

    // some GPX 1.0 backward compatibility
    QString url;
    readXml(xml, "url", url);
    if(!url.isEmpty())
    {
        link_t link;
        link.uri.setUrl(url);
        readXml(xml, "urlname", link.text);

        wpt.links << link;
    }

}


void IGisItem::writeWpt(QDomElement& xml, const wpt_t& wpt)
{
    QString str;

    str.sprintf("%1.8f", wpt.lat);
    xml.setAttribute("lat",str);
    str.sprintf("%1.8f", wpt.lon);
    xml.setAttribute("lon",str);

    writeXml(xml, "ele", wpt.ele);
    writeXml(xml, "time", wpt.time);
    writeXml(xml, "magvar", wpt.magvar);
    writeXml(xml, "geoidheight", wpt.geoidheight);
    writeXml(xml, "name", wpt.name);
    writeXml(xml, "cmt", wpt.cmt);
    writeXml(xml, "desc", wpt.desc);
    writeXml(xml, "src", wpt.src);
    writeXml(xml, "link", wpt.links);
    writeXml(xml, "sym", wpt.sym);
    writeXml(xml, "type", wpt.type);
    writeXml(xml, "fix", wpt.fix);
    writeXml(xml, "sat", wpt.sat);
    writeXml(xml, "hdop", wpt.hdop);
    writeXml(xml, "vdop", wpt.vdop);
    writeXml(xml, "pdop", wpt.pdop);
    writeXml(xml, "ageofdgpsdata", wpt.ageofdgpsdata);
    writeXml(xml, "dgpsid", wpt.dgpsid);

}