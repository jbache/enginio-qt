/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/qobject.h>
#include <QtCore/qthread.h>

#include <Enginio/enginioclient.h>
#include <Enginio/private/enginioclient_p.h>
#include <Enginio/enginioreply.h>
#include <Enginio/enginioidentity.h>

#include "../common/common.h"

// For this test to work, there needs to be a property "fileAttachment"
// for "objects.files" that is a ref to files.

class tst_Files: public QObject
{
    Q_OBJECT

    QString _backendName;
    EnginioTests::EnginioBackendManager _backendManager;
    QByteArray _backendId;
    QByteArray _backendSecret;

public slots:
    void error(EnginioReply *reply) {
        qDebug() << "\n\n### ERROR";
        qDebug() << reply->errorString();
        reply->dumpDebugInfo();
        qDebug() << "\n###\n";
    }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void fileUploadDownload_data();
    void fileUploadDownload();
};

void tst_Files::initTestCase()
{
    if (EnginioTests::TESTAPP_URL.isEmpty())
        QFAIL("Needed environment variable ENGINIO_API_URL is not set!");

    _backendName = QStringLiteral("Files") + QString::number(QDateTime::currentMSecsSinceEpoch());
    QVERIFY(_backendManager.createBackend(_backendName));

    QJsonObject apiKeys = _backendManager.backendApiKeys(_backendName, EnginioTests::TESTAPP_ENV);
    _backendId = apiKeys["backendId"].toString().toUtf8();
    _backendSecret = apiKeys["backendSecret"].toString().toUtf8();

    QVERIFY(!_backendId.isEmpty());
    QVERIFY(!_backendSecret.isEmpty());

    EnginioTests::prepareTestObjectType(_backendName);
}

void tst_Files::cleanupTestCase()
{
    QVERIFY(_backendManager.removeBackend(_backendName));
}

void tst_Files::fileUploadDownload_data()
{
    QTest::addColumn<int>("chunkSize");

    QTest::newRow("Multi Part") << -1;
    // With such a small chunk size the image will be uploaded in chunks
    QTest::newRow("Chunked") << 1024;
}

void tst_Files::fileUploadDownload()
{
    QFETCH(int, chunkSize);
    qDebug() << chunkSize;

    int replyCount = 0;

    EnginioClient client;
    QObject::connect(&client, SIGNAL(error(EnginioReply *)), this, SLOT(error(EnginioReply *)));
    client.setBackendId(_backendId);
    client.setBackendSecret(_backendSecret);
    client.setServiceUrl(EnginioTests::TESTAPP_URL);

    if (chunkSize > 0) {
        EnginioClientPrivate *clientPrivate = EnginioClientPrivate::get(&client);
        clientPrivate->_uploadChunkSize = chunkSize;
    }

    QSignalSpy spy(&client, SIGNAL(finished(EnginioReply *)));
    QSignalSpy spyError(&client, SIGNAL(error(EnginioReply*)));

    //![upload-create-object]
    QJsonObject obj;
    obj["objectType"] = QString::fromUtf8("objects.%1").arg(EnginioTests::CUSTOM_OBJECT1);
    obj["title"] = QString::fromUtf8("Object With File");
    const EnginioReply* createReply = client.create(obj);
    //![upload-create-object]
    QVERIFY(createReply);
    ++replyCount;
    QTRY_COMPARE(spy.count(), replyCount);
    QCOMPARE(spyError.count(), 0);

    const EnginioReply *responseObjectCreation = spy[0][0].value<EnginioReply*>();
    QCOMPARE(responseObjectCreation, createReply);
    QCOMPARE(responseObjectCreation->networkError(), QNetworkReply::NoError);
    QJsonObject data = responseObjectCreation->data();
    QVERIFY(!data.isEmpty());
    QCOMPARE(data["title"], obj["title"]);
    QCOMPARE(data["objectType"], obj["objectType"]);
    QString id = data["id"].toString();
    QVERIFY(!id.isEmpty());

    QString fileName = QStringLiteral("test.png");
    QString filePath = QStringLiteral(TEST_FILE_PATH);
    QVERIFY(QFile::exists(filePath));
    QString fileId;

    // Attach file to the object
    {
    //![upload]
    QJsonObject object;
    object["id"] = id;
    object["objectType"] = QString::fromUtf8("objects.%1").arg(EnginioTests::CUSTOM_OBJECT1);
    object["propertyName"] = QStringLiteral("fileAttachment");

    QJsonObject fileObject;
    fileObject[QStringLiteral("fileName")] = fileName;

    QJsonObject uploadJson;
    uploadJson[QStringLiteral("targetFileProperty")] = object;
    uploadJson[QStringLiteral("file")] = fileObject;
    const EnginioReply* responseUpload = client.uploadFile(uploadJson, QUrl(filePath));
    //![upload]

    QSignalSpy progressSpy(responseUpload, SIGNAL(progress(qint64,qint64)));
    QVERIFY(responseUpload);
    ++replyCount;
    QTRY_COMPARE(spy.count(), replyCount);
    QCOMPARE(spyError.count(), 0);
    fileId = responseUpload->data().value(QStringLiteral("id")).toString();
    QVERIFY(progressSpy.count() > 1);
    }

    // Query including files
    {
    QJsonObject obj2;
    obj2 = QJsonDocument::fromJson(
                "{\"include\": {\"fileAttachment\": {}},"
                 "\"objectType\": \"objects." + EnginioTests::CUSTOM_OBJECT1.toUtf8() + "\","
                 "\"query\": {\"id\": \"" + id.toUtf8() + "\"}}").object();

    const EnginioReply *reply = client.query(obj2);
    QVERIFY(reply);

    ++replyCount;
    QTRY_COMPARE(spy.count(), replyCount);
    QCOMPARE(spyError.count(), 0);
    const EnginioReply *responseQuery = spy[2][0].value<EnginioReply*>();
    data = responseQuery->data();
    QVERIFY(data["results"].isArray());
    QVERIFY(data["results"].toArray().first().toObject()["fileAttachment"].isObject());
    QVERIFY(!data["results"].toArray().first().toObject()["fileAttachment"].toObject()["url"].toString().isEmpty());
    QCOMPARE(data["results"].toArray().first().toObject()["fileAttachment"].toObject()["fileName"].toString(), fileName);

    QFile file(filePath);
    double fileSize = (double) file.size();
    QCOMPARE(data["results"].toArray().first().toObject()["fileAttachment"].toObject()["fileSize"].toDouble(), fileSize);
    QCOMPARE(data["results"].toArray().first().toObject()["fileAttachment"].toObject()["id"].toString(), fileId);
    }

    // Download
    {
    //![download]
    QJsonObject object;
    object["id"] = fileId; // ID of an existing object with attached file

    const EnginioReply* replyDownload = client.downloadFile(object);
    //![download]

    QVERIFY(replyDownload);
    ++replyCount;
    QTRY_COMPARE(spy.count(), replyCount);
    QCOMPARE(spyError.count(), 0);
    const EnginioReply *responseDownload = spy[3][0].value<EnginioReply*>();
    QJsonObject downloadData = responseDownload->data();

    QVERIFY(!downloadData["expiringUrl"].toString().isEmpty());
    QVERIFY(!downloadData["expiresAt"].toString().isEmpty());
    QNetworkAccessManager nam;
    QNetworkRequest req;
    req.setUrl(QUrl(downloadData["expiringUrl"].toString()));
    QNetworkReply *reply = nam.get(req);
    QVERIFY(reply);
    QSignalSpy downloadSpy(reply, SIGNAL(finished()));
    QTRY_COMPARE(downloadSpy.count(), 1);
    QByteArray imageData = reply->readAll();
    QImage img = QImage::fromData(imageData);
    qDebug() << "full size:" << imageData.size();
    QCOMPARE(img.size(), QSize(181, 54));
    }


    // View/Query the file details
    {
    QJsonObject fileObject;
    fileObject.insert("id", fileId);
    EnginioReply *fileInfo = client.query(fileObject, EnginioClient::FileOperation);
    QVERIFY(fileInfo);
    ++replyCount;
    QTRY_COMPARE(spy.count(), replyCount);
    QCOMPARE(spyError.count(), 0);
    QCOMPARE(fileInfo->data()["fileName"].toString(), fileName);
    QFile file(filePath);
    QCOMPARE(fileInfo->data()["fileSize"].toDouble(), (double)file.size());
    QVERIFY(fileInfo->data()["variants"].toObject().contains("thumbnail"));
    QString thumbnailStatus = fileInfo->data()["variants"].toObject()["thumbnail"].toObject()["status"].toString();
    int count = 0;
    while (thumbnailStatus == "processing" && ++count < 10) {
        QTest::qWait(1000);
        fileInfo = client.query(fileObject, EnginioClient::FileOperation);
        QVERIFY(fileInfo);
        ++replyCount;
        QTRY_COMPARE(spy.count(), replyCount);
        QCOMPARE(spyError.count(), 0);
        thumbnailStatus = fileInfo->data()["variants"].toObject()["thumbnail"].toObject()["status"].toString();
    }
    QCOMPARE(thumbnailStatus, EnginioString::complete);
    }


    // Download thumbnail
// Needs an image processor on the server
/*
{
    "thumbnail": {
        "crop": "20x20"
    }
}
*/
    {
    QJsonObject object;
    object["id"] = fileId; // ID of an existing object with attached file
    object[EnginioString::variant] = QStringLiteral("thumbnail");

    const EnginioReply* replyDownload = client.downloadFile(object);

    QVERIFY(replyDownload);
    ++replyCount;
    QTRY_COMPARE(spy.count(), replyCount);
    QCOMPARE(spyError.count(), 0);
    const EnginioReply *responseDownload = spy[3][0].value<EnginioReply*>();
    QJsonObject downloadData = responseDownload->data();

    QVERIFY(!downloadData["expiringUrl"].toString().isEmpty());
    QVERIFY(!downloadData["expiresAt"].toString().isEmpty());

    QNetworkAccessManager nam;
    QNetworkRequest req;
    req.setUrl(QUrl(downloadData["expiringUrl"].toString()));
    QNetworkReply *reply = nam.get(req);
    QVERIFY(reply);
    QSignalSpy downloadSpy(reply, SIGNAL(finished()));
    QTRY_COMPARE(downloadSpy.count(), 1);
    QByteArray imageData = reply->readAll();
    QImage img = QImage::fromData(imageData);
    qDebug() << img.size();
    QEXPECT_FAIL("", "The server returns the original image instead of thumbnail", Continue);
    QCOMPARE(img.size(), QSize(20, 20));
    }
}


QTEST_MAIN(tst_Files)
#include "tst_files.moc"
