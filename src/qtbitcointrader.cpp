﻿// Copyright (C) 2013 July IGHOR.
// I want to create Bitcoin Trader application that can be configured for any rule and strategy.
// If you want to help me please Donate: 1d6iMwjjNo8ZGYeJBZKXgcgVk9o7fXcjc
// For any questions please use contact form https://sourceforge.net/projects/bitcointrader/
// Or send e-mail directly to julyighor@gmail.com
//
// You may use, distribute and copy the Qt Bitcion Trader under the terms of
// GNU General Public License version 3

#include <QTimeLine>
#include <QScrollBar>
#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include "main.h"
#include "julylightchanges.h"
#include "julyspinboxfix.h"
#include "addrulewindow.h"
#include <QFileInfo>
#include <QClipboard>
#include <QProcess>
#include "feecalculator.h"
#include <QFile>
#include <QSysInfo>
#include <QProcess>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QUrl>
#include "aboutdialog.h"
#include "audioplayer.h"
#include "exchange_mtgox.h"
#include "exchange_btce.h"
#include <QSystemTrayIcon>

#ifdef Q_OS_WIN
#include "qtwin.h"
#endif

QtBitcoinTrader::QtBitcoinTrader()
	: QDialog()
{
	trayIcon=0;
	btcDecimals=8;
	usdDecimals=5;
	priceDecimals=5;
	isDetachedLog=false;
	isDetachedTrades=false;
	isDetachedRules=false;
	isDetachedCharts=false;

	forcedReloadOrders=true;
	firstRowGuid=-1;
	lastLoadedCurrency=-1;
	profitSellThanBuyUnlocked=true;
	profitBuyThanSellUnlocked=true;
	profitBuyThanSellChangedUnlocked=true;
	profitSellThanBuyChangedUnlocked=true;

	constructorFinished=false;
	appDir=QApplication::applicationDirPath()+"/";

	authErrorOnce=false;
	showingMessage=false;
	lastLagState=false;
	floatFee=0.0;
	floatFeeDec=0.0;
	floatFeeInc=0.0;
	marketPricesNotLoaded=true;
	balanceNotLoaded=true;
	sellLockBtcToSell=false;
	sellLockPricePerCoin=false;
	sellLockAmountToReceive=false;

	buyLockTotalBtc=false;
	buyLockPricePerCoin=false;
	buyLockTotalSpend=false;

	lastMarketLowPrice=0.0;
	lastMarketHighPrice=0.0;

	ui.setupUi(this);

	accountFeeChanged(ui.accountFee->value());
	setAttribute(Qt::WA_QuitOnClose,true);

	setWindowFlags(Qt::Window);

	{
		ui.usdLabelLastSell->setPixmap(QPixmap(":/Resources/CurrencySign/USD.png"));ui.usdLabelLastSell->setToolTip("USD");
		ui.usdLabelLastBuy->setPixmap(QPixmap(":/Resources/CurrencySign/USD.png"));ui.usdLabelLastBuy->setToolTip("USD");
	}

#ifdef Q_OS_WIN
	if(QtWin::isCompositionEnabled())
		QtWin::extendFrameIntoClientArea(this);
#endif

	setApiDown(false);

	ui.ordersTableFrame->setVisible(false);

	ui.ordersTable->horizontalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
	ui.ordersTable->horizontalHeader()->setResizeMode(1,QHeaderView::ResizeToContents);
	ui.ordersTable->horizontalHeader()->setResizeMode(2,QHeaderView::Stretch);
	ui.ordersTable->horizontalHeader()->setResizeMode(3,QHeaderView::ResizeToContents);
	ui.ordersTable->horizontalHeader()->setResizeMode(4,QHeaderView::ResizeToContents);
	ui.ordersTable->horizontalHeader()->setResizeMode(5,QHeaderView::ResizeToContents);

	ui.rulesNoMessage->setVisible(true);
	ui.rulesTable->setVisible(false);

	ui.rulesTable->horizontalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
	ui.rulesTable->horizontalHeader()->setResizeMode(1,QHeaderView::Stretch);
	ui.rulesTable->horizontalHeader()->setResizeMode(2,QHeaderView::ResizeToContents);
	ui.rulesTable->horizontalHeader()->setResizeMode(3,QHeaderView::ResizeToContents);
	ui.rulesTable->horizontalHeader()->setResizeMode(4,QHeaderView::ResizeToContents);

	ui.tableTrades->horizontalHeader()->setResizeMode(0,QHeaderView::Stretch);
	ui.tableTrades->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
	ui.tableTrades->horizontalHeader()->setResizeMode(1,QHeaderView::ResizeToContents);
	ui.tableTrades->horizontalHeader()->setResizeMode(2,QHeaderView::ResizeToContents);
	ui.tableTrades->horizontalHeader()->setResizeMode(3,QHeaderView::Stretch);
	ui.tableTrades->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);

	QSettings settings(iniFileName,QSettings::IniFormat);

	profileName=settings.value("ProfileName","Default Profile").toString();
	setWindowTitle(windowTitle()+" v"+appVerStr+" ["+profileName+"]");

	ui.sslCheck->setChecked(settings.value("OpenSSL",true).toBool());

	ui.accountBTCBeep->setChecked(settings.value("AccountBTCBeep",false).toBool());
	ui.accountUSDBeep->setChecked(settings.value("AccountUSDBeep",false).toBool());
	ui.marketHighBeep->setChecked(settings.value("MarketHighBeep",false).toBool());
	ui.marketLowBeep->setChecked(settings.value("MarketLowBeep",false).toBool());
	ui.ruleBeep->setChecked(settings.value("RuleExecutedBeep",false).toBool());

	ordersSelectionChanged();
	connect(ui.sslCheck,SIGNAL(toggled(bool)),this,SLOT(setSslEnabledSlot(bool)));

	new JulyLightChanges(ui.marketVolume);
	new JulyLightChanges(ui.marketBuy);
	new JulyLightChanges(ui.marketSell);
	new JulyLightChanges(ui.marketBuy);
	new JulyLightChanges(ui.marketHigh);
	new JulyLightChanges(ui.marketLow);
	new JulyLightChanges(ui.accountBTC);
	new JulyLightChanges(ui.accountUSD);
	new JulyLightChanges(ui.marketLast);
	new JulyLightChanges(ui.ordersLastSellPrice);
	new JulyLightChanges(ui.ordersLastBuyPrice);
	new JulyLightChanges(ui.tradesVolume5m);

	foreach(QDoubleSpinBox* spinBox, findChildren<QDoubleSpinBox*>())new JulySpinBoxFix(spinBox);

	QSettings settingsMain(appDataDir+"/Settings.set",QSettings::IniFormat);
	checkForUpdates=settingsMain.value("CheckForUpdates",true).toBool();

	int screenCount=QApplication::desktop()->screenCount();
	QPoint cursorPos=QCursor::pos();
	QRect currentDesktopRect(0,0,1024,720);
	for(int n=0;n<screenCount;n++)
	{
		if(QApplication::desktop()->screen(n)->rect().contains(cursorPos))
			currentDesktopRect=QApplication::desktop()->availableGeometry(n);
	}
	
	ui.tabOrdersLogOnTop->setVisible(false);
	ui.tabRulesOnTop->setVisible(false);
	ui.tabTradesOnTop->setVisible(false);
	ui.tabChartsOnTop->setVisible(false);

	ui.tabOrdersLog->installEventFilter(this);
	ui.tabRules->installEventFilter(this);
	ui.tabLastTrades->installEventFilter(this);
	ui.tabCharts->installEventFilter(this);

#ifdef GENERATE_LANGUAGE_FILE
	julyTranslator->loadMapFromUi(this);
	julyTranslator->saveToFile("LanguageDefault.lng");
#endif
	exchangeId=settings.value("ExchangeId",0).toInt();
	switch(exchangeId)
	{
	case 1:
		{//BTC-E
			btcDecimals=8;
			usdDecimals=8;
			priceDecimals=3;
			exchangeName="BTC-E";
			ui.loginVolumeBack->setVisible(false);
			ui.exchangeLagBack->setVisible(false);

			QFile curMap(":/Resources/CurrenciesBTCe.map");
			curMap.open(QIODevice::ReadOnly);
			QStringList curList=QString(curMap.readAll().replace("\r","")).split("\n");
			curMap.close();
			for(int n=0;n<curList.count();n++)
			{
				QStringList curPair=curList.at(n).split("=");
				if(curPair.count()==2)ui.currencyComboBox->insertItem(ui.currencyComboBox->count(),curPair.first(),curPair.last());
			}

			(new Exchange_BTCe(restSign,restKey))->setupApi(this,false,ui.sslCheck->isChecked());
		}break;
	default:
		{
			btcDecimals=8;
			usdDecimals=5;
			priceDecimals=5;
			QFile curMap(":/Resources/CurrenciesMtGox.map");
			curMap.open(QIODevice::ReadOnly);
			QStringList curList=QString(curMap.readAll().replace("\r","")).split("\n");
			curMap.close();
			for(int n=0;n<curList.count();n++)
			{
				QStringList curPair=curList.at(n).split("=");
				if(curPair.count()==2)ui.currencyComboBox->insertItem(ui.currencyComboBox->count(),curPair.first(),curPair.last());
			}

			exchangeName="Mt.Gox"; (new Exchange_MtGox(restSign,restKey))->setupApi(this,false,ui.sslCheck->isChecked());
		}
	}

	reloadLanguageList();
	resize(qMax(minimumSizeHint().width(),qMin(1024,(int)(currentDesktopRect.width()*0.95))),qMin((int)(currentDesktopRect.height()*0.95),700));
	constructorFinished=true;

	checkValidRulesButtons();
	currencyChanged(ui.currencyComboBox->currentIndex());
	connect(julyTranslator,SIGNAL(languageChanged()),this,SLOT(languageChanged()));

	if(checkForUpdates)QProcess::startDetached(QApplication::applicationFilePath(),QStringList("/checkupdate"));
	if(ui.langComboBox->count()==0)fixAllChildButtonsAndLabels(this);
	secondSlot();
}

QtBitcoinTrader::~QtBitcoinTrader()
{
	if(trayIcon)trayIcon->hide();
}

void QtBitcoinTrader::buttonMinimizeToTray()
{
	if(trayIcon==0)
	{
		trayIcon=new QSystemTrayIcon(QIcon(":/Resources/QtBitcoinTrader.png"),this);
		trayIcon->setToolTip(windowTitle());
		connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
	}
	trayIcon->show();
	hide();
	if(ui.tabOrdersLog->parent()==0)ui.tabOrdersLog->hide();
	if(ui.tabLastTrades->parent()==0)ui.tabLastTrades->hide();
	if(ui.tabRules->parent()==0)ui.tabRules->hide();
	if(ui.tabCharts->parent()==0)ui.tabCharts->hide();
}

void QtBitcoinTrader::trayActivated(QSystemTrayIcon::ActivationReason)
{
	if(trayIcon==0)return;
	show();
	if(ui.tabOrdersLog->parent()==0)ui.tabOrdersLog->show();
	if(ui.tabLastTrades->parent()==0)ui.tabLastTrades->show();
	if(ui.tabRules->parent()==0)ui.tabRules->show();
	if(ui.tabCharts->parent()==0)ui.tabCharts->show();
	trayIcon->hide();
	delete trayIcon; trayIcon=0;
}

void QtBitcoinTrader::tabLogOrdersOnTop(bool on)
{
	if(!isDetachedLog)return;
	ui.tabOrdersLog->hide();
	if(on)ui.tabOrdersLog->setWindowFlags(Qt::Window|Qt::WindowStaysOnTopHint);
	else  ui.tabOrdersLog->setWindowFlags(Qt::Window);
	ui.tabOrdersLog->show();
}

void QtBitcoinTrader::tabRulesOnTop(bool on)
{
	if(!isDetachedRules)return;
	ui.tabRules->hide();
	if(on)ui.tabRules->setWindowFlags(Qt::Window|Qt::WindowStaysOnTopHint);
	else  ui.tabRules->setWindowFlags(Qt::Window);
	ui.tabRules->show();
}

void QtBitcoinTrader::tabTradesOnTop(bool on)
{
	if(!isDetachedTrades)return;
	ui.tabLastTrades->hide();
	if(on)ui.tabLastTrades->setWindowFlags(Qt::Window|Qt::WindowStaysOnTopHint);
	else  ui.tabLastTrades->setWindowFlags(Qt::Window);
	ui.tabLastTrades->show();
}

void QtBitcoinTrader::tabChartsOnTop(bool on)
{
	if(!isDetachedCharts)return;
	ui.tabCharts->hide();
	if(on)ui.tabCharts->setWindowFlags(Qt::Window|Qt::WindowStaysOnTopHint);
	else  ui.tabCharts->setWindowFlags(Qt::Window);
	ui.tabCharts->show();
}

bool QtBitcoinTrader::isValidGeometry(QRect *geo)
{
	static QRect allDesktopsRect=QApplication::desktop()->geometry();
	return geo->width()>100&&geo->height()>100&&allDesktopsRect.contains(geo->topLeft())&&allDesktopsRect.contains(geo->bottomRight());
}

void QtBitcoinTrader::loadUiSettings()
{
	QSettings settings(iniFileName,QSettings::IniFormat);

	if(settings.value("DetachedLog",isDetachedLog).toBool())detachLog();

	if(settings.value("DetachedRules",isDetachedRules).toBool())detachRules();
	
	if(settings.value("DetachedTrades",isDetachedRules).toBool())detachTrades();

	if(settings.value("DetachedCharts",isDetachedRules).toBool())detachCharts();
	
	int savedTab=settings.value("TradesCurrentTab",0).toInt();
	if(savedTab<ui.tabWidget->count())ui.tabWidget->setCurrentIndex(savedTab);
	settings.sync();
}

void QtBitcoinTrader::checkUpdate()
{
    QProcess::startDetached(QApplication::applicationFilePath(),QStringList("/checkupdatemessage"));
}

void QtBitcoinTrader::resizeEvent(QResizeEvent *event)
{
	event->accept();
	fixWindowMinimumSize();
}

void QtBitcoinTrader::addLastTrade(double btcDouble, qint64 dateT, double usdDouble, QByteArray curRency)
{
	int goingUp=-1;
	double marketLast=ui.marketLast->value();
	if(marketLast<usdDouble)goingUp=1;else
	if(marketLast==usdDouble)goingUp=0;

	ui.marketLast->setValue(usdDouble);

	ui.tradesVolume5m->setValue(ui.tradesVolume5m->value()+btcDouble);
	QString btcValue=currencyASign+" "+QString::number(btcDouble,'f',btcDecimals);
	QString usdValue=currencySignMap->value(curRency.toUpper(),"USD")+" "+QString::number(usdDouble,'f',priceDecimals);
	QString dateValue=QDateTime::fromTime_t(dateT).toString(localDateTimeFormat);
	ui.tableTrades->insertRow(0);
	static QColor btcColor("#996515");
	QTableWidgetItem *newItem=new QTableWidgetItem(btcValue);newItem->setTextColor(btcColor);
	newItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
	ui.tableTrades->setItem(0,0,newItem);
	newItem->setData(Qt::UserRole,btcDouble);
	newItem=new QTableWidgetItem(dateValue);newItem->setTextColor(Qt::gray);
	newItem->setTextAlignment(Qt::AlignCenter);
	newItem->setData(Qt::UserRole,dateT);
	ui.tableTrades->setItem(0,1,newItem);
	newItem=new QTableWidgetItem(usdValue);newItem->setTextColor(Qt::darkGreen);
	newItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
	ui.tableTrades->setItem(0,2,newItem);

	static QPixmap arrowUp(":/Resources/TradeUp.png");
	static QPixmap arrowDown(":/Resources/TradeDown.png");
	if(goingUp==1)ui.tableTrades->setItem(0,3,new QTableWidgetItem(arrowUp,""));else
	if(goingUp==-1)ui.tableTrades->setItem(0,3,new QTableWidgetItem(arrowDown,""));
	
	if(ui.tradesAutoScrollCheck->isChecked()&&ui.tabLastTrades->isVisible())
	{
		setTradesScrollBarValue(ui.tableTrades->verticalScrollBar()->value()+ui.tableTrades->rowHeight(0));
		tabTradesScrollUp();
	}
}

void QtBitcoinTrader::clearTimeOutedTrades()
{
	if(ui.tableTrades->rowCount()==0)return;
	qint64 min5Date=QDateTime::currentDateTime().addSecs(-300).toTime_t();
	while(ui.tableTrades->rowCount()&&ui.tableTrades->item(ui.tableTrades->rowCount()-1,1)->data(Qt::UserRole).toLongLong()<min5Date)
	{
		ui.tradesVolume5m->setValue(ui.tradesVolume5m->value()-ui.tableTrades->item(ui.tableTrades->rowCount()-1,0)->data(Qt::UserRole).toDouble());
		ui.tableTrades->removeRow(ui.tableTrades->rowCount()-1);
	}
}

void QtBitcoinTrader::secondSlot()
{
	static QTimer secondTimer(this);
	if(secondTimer.interval()==0)
	{
		secondTimer.setInterval(1000);
		secondTimer.setSingleShot(true);
		connect(&secondTimer,SIGNAL(timeout()),this,SLOT(secondSlot()));
	}
	clearTimeOutedTrades();
	secondTimer.start(1000);
}

void QtBitcoinTrader::tabTradesScrollUp()
{
	static QTimeLine timeLine(1,this);
	if(timeLine.duration()==1)
	{
		connect(&timeLine,SIGNAL(frameChanged(int)),this,SLOT(setTradesScrollBarValue(int)));
		timeLine.setDuration(600);
		timeLine.setEasingCurve(QEasingCurve::OutCubic);
		timeLine.setLoopCount(1);
	}
	else
	timeLine.stop();
	int currentScrollPos=ui.tableTrades->verticalScrollBar()->value();
	if(currentScrollPos==0)return;
	timeLine.setFrameRange(currentScrollPos,0);
	timeLine.start();
}

void QtBitcoinTrader::tabTradesIndexChanged(int)
{
	if(ui.tabLastTrades->isVisible()&&ui.tradesAutoScrollCheck->isChecked())tabTradesScrollUp();
}

void QtBitcoinTrader::setTradesScrollBarValue(int val)
{
	ui.tableTrades->verticalScrollBar()->setValue(val);
}

void QtBitcoinTrader::fillAllUsdLabels(QWidget *par, QString curName)
{
	curName=curName.toUpper();
	QPixmap btcPixmap("://Resources/CurrencySign/"+curName+".png");
	foreach(QLabel* labels, par->findChildren<QLabel*>())
		if(labels->accessibleDescription()=="USD_ICON")
		{
			labels->setPixmap(btcPixmap);
			labels->setToolTip(curName);
		}
}
void QtBitcoinTrader::fillAllBtcLabels(QWidget *par, QString curName)
{
	curName=curName.toUpper();
	QPixmap btcPixmap("://Resources/CurrencySign/"+curName+".png");
	foreach(QLabel* labels, par->findChildren<QLabel*>())
		if(labels->accessibleDescription()=="BTC_ICON")
		{
			labels->setPixmap(btcPixmap);
			labels->setToolTip(curName);
		}
}

void QtBitcoinTrader::makeRitchValue(QString *text)
{
	int lastSymbol=text->length()-1;
	if(lastSymbol==-1)return;
	while(lastSymbol>-1&&text->at(lastSymbol)=='0')lastSymbol--;
	if(lastSymbol>-1&&text->at(lastSymbol)=='.')lastSymbol--;
	if(lastSymbol<-1)return;
	QString buff=text->left(lastSymbol+1);
	text->remove(0,lastSymbol+1);
	text->prepend("<font color=gray>");
	text->append("</font>");
	text->prepend(buff);
}

void QtBitcoinTrader::reloadLanguageList(QString preferedLangFile)
{
	if(preferedLangFile.isEmpty())preferedLangFile=julyTranslator->lastFile();
	if(!QFile::exists(preferedLangFile))preferedLangFile.clear();
	constructorFinished=false;
	ui.langComboBox->clear();

	QStringList langList;
	QFile resLanguage(":/Resources/Language/LangList.ini");
	resLanguage.open(QIODevice::ReadOnly);
	QStringList resourceLanguages=QString(resLanguage.readAll()).split("\r\n");
	for(int n=0;n<resourceLanguages.count();n++)if(!resourceLanguages.at(n).isEmpty())langList<<":/Resources/Language/"+resourceLanguages.at(n);
	QStringList folderLangList=QDir(appDataDir+"Language","*.lng").entryList();
	folderLangList.sort();
	for(int n=0;n<folderLangList.count();n++)langList<<appDataDir+"Language/"+folderLangList.at(n);
	int selectedLangId=-1;
	if(preferedLangFile.isEmpty()||!preferedLangFile.isEmpty()&&!QFile::exists(preferedLangFile))preferedLangFile=defaultLangFile;
	for(int n=0;n<langList.count();n++)
	{
		JulyTranslator translateName;
		translateName.loadFromFile(langList.at(n));
		QString langName=translateName.translateString("LANGUAGE_NAME","");
		if(langName.isEmpty())continue;
		if(preferedLangFile==langList.at(n))selectedLangId=n;
		ui.langComboBox->insertItem(ui.langComboBox->count(),langName,langList.at(n));
	}
	if(selectedLangId>-1)ui.langComboBox->setCurrentIndex(selectedLangId);
	julyTranslator->loadFromFile(preferedLangFile);
	constructorFinished=true;
	languageChanged();
}

void QtBitcoinTrader::languageComboBoxChanged(int val)
{
	if(val<0||!constructorFinished)return;
	QString loadFromFile=ui.langComboBox->itemData(val,Qt::UserRole).toString();
	if(loadFromFile.isEmpty())return;
	julyTranslator->loadFromFile(loadFromFile);
	QSettings settings(appDataDir+"/Settings.set",QSettings::IniFormat);
	settings.setValue("LanguageFile",loadFromFile);
}

void QtBitcoinTrader::fixAllChildButtonsAndLabels(QWidget *par)
{
	foreach(QPushButton* pushButtons, par->findChildren<QPushButton*>())
		pushButtons->setMinimumWidth(qMin(pushButtons->maximumWidth(),QFontMetrics(pushButtons->font()).width(pushButtons->text())+10));

	foreach(QCheckBox* checkBoxes, par->findChildren<QCheckBox*>())
		checkBoxes->setMinimumWidth(qMin(checkBoxes->maximumWidth(),QFontMetrics(checkBoxes->font()).width(checkBoxes->text())+20));

	foreach(QGroupBox* groupBox, par->findChildren<QGroupBox*>())
		if(groupBox->maximumWidth()>1000)
			groupBox->setMinimumWidth(qMax(groupBox->minimumSizeHint().width(),QFontMetrics(groupBox->font()).width(groupBox->title())+20));

	foreach(QLabel* labels, par->findChildren<QLabel*>())
		if(labels->text().length()&&labels->text().at(0)!='<')
			labels->setMinimumWidth(qMin(labels->maximumWidth(),QFontMetrics(labels->font()).width(labels->text())));

	foreach(QDoubleSpinBox* spinBox, par->findChildren<QDoubleSpinBox*>())
	{
		if(spinBox->accessibleName()=="BTC")spinBox->setDecimals(btcDecimals);
		else
		if(spinBox->accessibleName()=="USD")spinBox->setDecimals(usdDecimals);
		else
		if(spinBox->accessibleName()=="PRICE")spinBox->setDecimals(priceDecimals);
	}

	QSize minSizeHint=par->minimumSizeHint();
	if(isValidSize(&minSizeHint))
	{
	par->setMinimumSize(par->minimumSizeHint());
	if(par->width()<par->minimumSizeHint().width())par->resize(par->minimumSizeHint().width(),par->height());
	}
}

void QtBitcoinTrader::loginChanged(QString text)
{
	if(text==" ")text=profileName;
	ui.accountLoginLabel->setText(text);
	ui.accountLoginLabel->setMinimumWidth(QFontMetrics(ui.accountLoginLabel->font()).width(text)+20);
	fixWindowMinimumSize();
}

void QtBitcoinTrader::currencyChanged(int val)
{
	if(!constructorFinished||val<0)return;
	if(val!=lastLoadedCurrency)lastLoadedCurrency=val;else return;

	QStringList curPair=ui.currencyComboBox->currentText().split("/");

	bool currencyAChanged=curPair.first()!=currencyAStr;
	currencyAStr=curPair.first().toAscii();
	currencyAStrLow=currencyAStr.toLower();
	currencyASign=currencySignMap->value(currencyAStr,"$");

	bool currencyBChanged=curPair.last()!=currencyBStr;
	currencyBStr=curPair.last().toAscii();
	currencyBStrLow=currencyBStr.toLower();
	currencyBSign=currencySignMap->value(currencyBStr,"$");
	fillAllUsdLabels(this,currencyBStr);
	fillAllBtcLabels(this,currencyAStr);

	currencyRequestPair=ui.currencyComboBox->itemData(val,Qt::UserRole).toByteArray();

	QSettings settings(iniFileName,QSettings::IniFormat);
	settings.setValue("Currency",QString(currencyBStr).toUpper());
	if(currencyAChanged)ui.accountBTC->setValue(0.0);
	if(currencyBChanged)ui.accountUSD->setValue(0.0);
	ui.marketBuy->setValue(0.0);
	ui.marketSell->setValue(0.0);
	ui.marketHigh->setValue(0.0);
	ui.marketLow->setValue(0.0);
	ui.marketLast->setValue(0.0);
	ui.marketVolume->setValue(0.0);
	ui.buyTotalSpend->setValue(0.0);
	ui.sellTotalBtc->setValue(0.0);
	ui.buyPricePerCoin->setValue(100);
	ui.sellPricePerCoin->setValue(200);
	ui.tableTrades->clearContents();
	ui.tableTrades->setRowCount(0);
	ui.tradesVolume5m->setValue(0.0);

	QString curCurrencyName=currencyNamesMap->value(currencyAStr,"BITCOINS");
	ui.buyGroupbox->setTitle(julyTr("GROUPBOX_BUY","Buy %1").arg(curCurrencyName));
	ui.sellGroupBox->setTitle(julyTr("GROUPBOX_SELL","Sell %1").arg(curCurrencyName));

	emit clearValues();
	marketPricesNotLoaded=true;
	balanceNotLoaded=true;
}

void QtBitcoinTrader::firstTicker()
{
	sellPricePerCoinAsMarketPrice();
	buyPricePerCoinAsMarketPrice();
}

void QtBitcoinTrader::firstAccInfo()
{
	sellTotalBtcToSellAllIn();
	buyBtcToBuyAllIn();
}

void QtBitcoinTrader::profitSellThanBuyCalc()
{
	if(!profitSellThanBuyUnlocked)return;
	profitSellThanBuyUnlocked=false;
	double calcValue=0.0;
	if(ui.sellTotalBtc->value()!=0.0&&ui.buyTotalBtcResult->value()!=0.0)
		calcValue=ui.buyTotalBtcResult->value()-ui.sellTotalBtc->value();
	ui.sellThanBuySpinBox->setValue(calcValue);
	profitSellThanBuyUnlocked=true;
}

void QtBitcoinTrader::profitBuyThanSellCalc()
{
	if(!profitBuyThanSellUnlocked)return;
	profitBuyThanSellUnlocked=false;
	double calcValue=0.0;
	if(ui.buyTotalSpend->value()!=0.0&&ui.sellAmountToReceive->value()!=0.0)
		calcValue=ui.sellAmountToReceive->value()-ui.buyTotalSpend->value();
	ui.profitLossSpinBox->setValue(calcValue);
	profitBuyThanSellUnlocked=true;
}

void QtBitcoinTrader::profitBuyThanSellPrecChanged(double val)
{
	if(profitBuyThanSellChangedUnlocked)
	{
		profitBuyThanSellChangedUnlocked=false;
		ui.profitLossSpinBox->setValue(val==0.0?0.0:ui.buyTotalSpend->value()*val/100.0);
		profitBuyThanSellChangedUnlocked=true;
	}
}

void QtBitcoinTrader::profitBuyThanSellChanged(double val)
{
	QString styleChanged;
	if(val<-0.009)styleChanged="QDoubleSpinBox {background: #ffaaaa;}";
	else
	if(val>0.009)styleChanged="QDoubleSpinBox {background: #aaffaa;}";

	if(profitBuyThanSellChangedUnlocked)
	{
		profitBuyThanSellChangedUnlocked=false;
		ui.profitLossSpinBoxPrec->setValue(ui.buyTotalSpend->value()==0.0?0.0:val*100.0/ui.buyTotalSpend->value());
		profitBuyThanSellChangedUnlocked=true;
	}

	ui.profitLossSpinBox->setStyleSheet(styleChanged);
	ui.profitLossSpinBoxPrec->setStyleSheet(styleChanged);
}

void QtBitcoinTrader::profitSellThanBuyPrecChanged(double val)
{
	if(profitBuyThanSellChangedUnlocked)
	{
		profitBuyThanSellChangedUnlocked=false;
		ui.sellThanBuySpinBox->setValue(val==0.0?0.0:ui.sellTotalBtc->value()*val/100.0);
		profitBuyThanSellChangedUnlocked=true;
	}
}

void QtBitcoinTrader::profitSellThanBuyChanged(double val)
{
	QString styleChanged;
	if(val<-0.009)styleChanged="QDoubleSpinBox {background: #ffaaaa;}";
	else
	if(val>0.009)styleChanged="QDoubleSpinBox {background: #aaffaa;}";

	if(profitBuyThanSellChangedUnlocked)
	{
		profitBuyThanSellChangedUnlocked=false;
		ui.sellThanBuySpinBoxPrec->setValue(ui.sellTotalBtc->value()==0.0?0.0:val*100.0/ui.sellTotalBtc->value());
		profitBuyThanSellChangedUnlocked=true;
	}

	ui.sellThanBuySpinBox->setStyleSheet(styleChanged);
	ui.sellThanBuySpinBoxPrec->setStyleSheet(styleChanged);
}

void QtBitcoinTrader::zeroSellThanBuyProfit()
{
	ui.sellThanBuySpinBox->setValue(0.0);
}

void QtBitcoinTrader::zeroBuyThanSellProfit()
{
	ui.profitLossSpinBox->setValue(0.0);
}

void QtBitcoinTrader::profitSellThanBuy()
{
	profitSellThanBuyUnlocked=false;
	ui.buyTotalSpend->setValue(ui.sellAmountToReceive->value());
	ui.buyPricePerCoin->setValue(ui.buyTotalSpend->value()/((ui.sellTotalBtc->value()+ui.sellThanBuySpinBox->value())/floatFeeDec));
	profitSellThanBuyUnlocked=true;
	profitBuyThanSellCalc();
	profitSellThanBuyCalc();
}

void QtBitcoinTrader::profitBuyThanSell()
{
	profitBuyThanSellUnlocked=false;
	ui.sellTotalBtc->setValue(ui.buyTotalBtcResult->value());
	ui.sellPricePerCoin->setValue((ui.buyTotalSpend->value()+ui.profitLossSpinBox->value())/(ui.sellTotalBtc->value()*floatFeeDec));
	profitBuyThanSellUnlocked=true;
	profitBuyThanSellCalc();
	profitSellThanBuyCalc();
}

void QtBitcoinTrader::setApiDown(bool on)
{
	ui.lagValue->setVisible(!on);
	ui.apiDownLabel->setVisible(on);
}

void QtBitcoinTrader::setSslEnabledSlot(bool on)
{
	emit setSslEnabled(on);
	QSettings settings(iniFileName,QSettings::IniFormat);
	settings.setValue("OpenSSL",on);
}

QString QtBitcoinTrader::clearData(QString data)
{
	while(data.count()&&(data.at(0)=='{'||data.at(0)=='['||data.at(0)=='\"'))data.remove(0,1);
	while(data.count()&&(data.at(data.length()-1)=='}'||data.at(data.length()-1)==']'||data.at(data.length()-1)=='\"'))data.remove(data.length()-1,1);
	return data;
}

void QtBitcoinTrader::saveSoundToggles()
{
	if(!constructorFinished)return;
	QSettings settings(iniFileName,QSettings::IniFormat);
	settings.setValue("AccountBTCBeep",ui.accountBTCBeep->isChecked());
	settings.setValue("AccountUSDBeep",ui.accountUSDBeep->isChecked());
	settings.setValue("MarketHighBeep",ui.marketHighBeep->isChecked());
	settings.setValue("MarketLowBeep",ui.marketLowBeep->isChecked());
	settings.setValue("RuleExecutedBeep",ui.ruleBeep->isChecked());
}

void QtBitcoinTrader::accountFeeChanged(double val)
{
	floatFee=val/100;
	floatFeeDec=1.0-floatFee;                 
	floatFeeInc=1.0+floatFee;
}

QByteArray QtBitcoinTrader::getMidData(QString a, QString b,QByteArray *data)
{
	QByteArray rez;
	if(b.isEmpty())b="\",";
	int startPos=data->indexOf(a,0);
	if(startPos>-1)
	{
	int endPos=data->indexOf(b,startPos+a.length());
	if(endPos>-1)rez=data->mid(startPos+a.length(),endPos-startPos-a.length());
	}
	return rez;
}

void QtBitcoinTrader::fixWindowMinimumSize()
{
	static QTime lastFixedTime;
	if(lastFixedTime.elapsed()<500)return;
	ui.marketGroupBox->setMinimumWidth(ui.marketGroupBox->minimumSizeHint().width());
	ui.buyThenSellGroupBox->setMinimumWidth(ui.buyThenSellGroupBox->minimumSizeHint().width());
	ui.sellThenBuyGroupBox->setMinimumWidth(ui.sellThenBuyGroupBox->minimumSizeHint().width());
	ui.groupBoxAccount->setMinimumWidth(ui.groupBoxAccount->minimumSizeHint().width());
	QSize minSizeHint=minimumSizeHint();
	if(isValidSize(&minSizeHint))setMinimumSize(minSizeHint);
	lastFixedTime.restart();
}

void QtBitcoinTrader::mtgoxLagChanged(double val)
{
	if(val>=1.0)
		ui.lagValue->setStyleSheet("QDoubleSpinBox {background: #ffaaaa;}");
	else
		ui.lagValue->setStyleSheet("");
}

void QtBitcoinTrader::updateLogTable()
{
	emit getHistory(false);
}

void QtBitcoinTrader::balanceChanged(double)
{
	checkValidSellButtons();
	checkValidBuyButtons();
	emit getHistory(false);
}

void QtBitcoinTrader::ordersIsEmpty()
{
	if(ui.ordersTable->rowCount())
	{
		if(isLogEnabled)logThread->writeLog("Order table cleared");
		oidMap.clear();
		ui.ordersTable->clearContents();
		ui.ordersTable->setRowCount(0);
		ui.ordersTableFrame->setVisible(false);
		ui.noOpenedOrdersLabel->setVisible(true);
		ui.noOpenedOrdersLabel->setVisible(true);
	}
}

void QtBitcoinTrader::orderCanceled(QByteArray oid)
{
	if(isLogEnabled)logThread->writeLog("Removed order: "+oid);

	for(int n=0;n<ui.ordersTable->rowCount();n++)
		if(ui.ordersTable->item(n,0)->data(Qt::UserRole).toByteArray()==oid)
		{
			ui.ordersTable->item(n,2)->setData(Qt::UserRole,"canceled");
			ui.ordersTable->item(n,2)->setText(julyTr("ORDER_STATE_CANCELED","canceled"));
			setOrdersTableRowState(n,0);
			break;
		}
}

void QtBitcoinTrader::ordersChanged(QString ordersData)
{
	QStringList ordersList=ordersData.split("\n");
	QMap<QByteArray,bool> activeOrders;
	for(int n=0;n<ordersList.count();n++)
	{
		//itemDate+";"+itemType+";"+itemStatus+";"+itemAmount+";"+itemPrice+";"+orderSign
		QString oidData=ordersList.at(n);
		QStringList oidDataList=oidData.split(";");
		if(oidDataList.count()!=7)continue;
		QByteArray oid=oidDataList.first().toAscii();
		oidDataList.removeFirst();
		oidData=oidDataList.join(";");

		QString findedData=oidMap.value(oid,"");

		if(findedData!="")//Update
		{
			if(forcedReloadOrders||findedData!=oidData)
				for(int n=0;n<ui.ordersTable->rowCount();n++)
					if(ui.ordersTable->item(n,0)->data(Qt::UserRole).toByteArray()==oid)
					{
						if(ui.ordersTable->item(n,2)->data(Qt::UserRole).toString()!="canceled")
						{
							if(forcedReloadOrders)ui.ordersTable->item(n,1)->setText(julyTr("ORDER_TYPE_"+oidDataList.at(1).toUpper(),oidDataList.at(1)));
							ui.ordersTable->item(n,2)->setText(julyTr("ORDER_STATE_"+oidDataList.at(2).toUpper(),oidDataList.at(2)));
							ui.ordersTable->item(n,3)->setText(currencyASign+" "+oidDataList.at(3));
							ui.ordersTable->item(n,4)->setText(oidDataList.at(5)+" "+QString::number(oidDataList.at(4).toDouble(),'f',priceDecimals));
							ui.ordersTable->item(n,5)->setText(oidDataList.at(5)+" "+QString::number(oidDataList.at(3).toDouble()*oidDataList.at(4).toDouble(),'f',usdDecimals));
							setOrdersTableRowStateByText(n,oidDataList.at(2).toAscii());
							oidMap[oid]=oidData;
						}
						break;
					}
		}
		else//Insert
		{
			insertIntoTable(oid,oidData);
			oidMap[oid]=oidData;
		}
		activeOrders[oid]=1;
	}

	for(int n=0;n<ui.ordersTable->rowCount();n++)
	{
		if(n<0)break;
		QByteArray curOid=ui.ordersTable->item(n,0)->data(Qt::UserRole).toByteArray();
		if(activeOrders.values(curOid).count()==0)
		{
			if(isLogEnabled)logThread->writeLog("Deleted: "+curOid);
			oidMap.remove(curOid);
			ui.ordersTable->removeRow(n--);
			ui.ordersTableFrame->setVisible(ui.ordersTable->rowCount());
			ui.noOpenedOrdersLabel->setVisible(!ui.ordersTable->isVisible());
		}
	}
	forcedReloadOrders=false;
}

void QtBitcoinTrader::identificationRequired()
{
	if(!showingMessage)
	{
		if(!showingMessage&&!authErrorOnce)
		{
			authErrorOnce=true;
			showingMessage=true;
			QMessageBox::warning(this,julyTr("AUTH_ERROR",exchangeName+" Error"),julyTr("TRUNAUTHORIZED","Identification required to access private API.\nPlease enter valid API key and Secret.\n\nTo get new API Keys:\nGo http://mtgox.com => \"Security Center\"  => \"Advanced API Key Creation\"\nCheck \"Info\" and \"Trade\"\nPress Create Key\nCopy and paste RestKey and RestSign to program."));
			if(isLogEnabled)logThread->writeLog(exchangeName.toAscii()+" Error: Identification required to access private API\nPlease restart application, and enter valid API key and Secret");
			showingMessage=false;
		}
	}
}

void QtBitcoinTrader::ordersLogChanged(QString newLog)
{
	ui.logTextEdit->setHtml(newLog);
	if(isLogEnabled)logThread->writeLog("Log Table Updated");
}

void QtBitcoinTrader::accLastSellChanged(QByteArray priceCurrency, double val)
{
	ui.ordersLastSellPrice->setValue(val);
	if(ui.usdLabelLastSell->toolTip()!=priceCurrency)
	{
		ui.usdLabelLastSell->setPixmap(QPixmap(":/Resources/CurrencySign/"+currencySignMap->key(priceCurrency.toUpper(),"$")+".png"));
		ui.usdLabelLastSell->setToolTip(priceCurrency);
	}
}

void QtBitcoinTrader::accLastBuyChanged(QByteArray priceCurrency, double val)
{
	ui.ordersLastBuyPrice->setValue(val);
	if(ui.usdLabelLastBuy->toolTip()!=priceCurrency)
	{
		ui.usdLabelLastBuy->setPixmap(QPixmap(":/Resources/CurrencySign/"+currencySignMap->key(priceCurrency.toUpper(),"$")+".png"));
		ui.usdLabelLastBuy->setToolTip(priceCurrency);
	}
}

void QtBitcoinTrader::translateUnicodeStr(QString *str)
{
	const QRegExp rx("(\\\\u[0-9a-fA-F]{4})");
	int pos=0;
	while((pos=rx.indexIn(*str,pos))!=-1)str->replace(pos++, 6, QChar(rx.cap(1).right(4).toUShort(0, 16)));
}

void QtBitcoinTrader::insertIntoTable(QByteArray oid, QString data)
{
	QStringList dataList=data.split(";");
	if(dataList.count()!=6)return;
	int curRow=ui.ordersTable->rowCount();
	QByteArray orderSign=dataList.at(5).toAscii();
	ui.ordersTable->setRowCount(curRow+1);
	ui.ordersTable->setRowHeight(curRow,30);
	ui.ordersTable->setItem(curRow,0,new QTableWidgetItem(QDateTime::fromTime_t(dataList.at(0).toUInt()).toString(localDateTimeFormat)));postWorkAtTableItem(ui.ordersTable->item(curRow,0));ui.ordersTable->item(curRow,0)->setData(Qt::UserRole,oid);
	ui.ordersTable->setItem(curRow,1,new QTableWidgetItem(julyTr("ORDER_TYPE_"+dataList.at(1).toUpper(),dataList.at(1))));postWorkAtTableItem(ui.ordersTable->item(curRow,1));
	ui.ordersTable->setItem(curRow,2,new QTableWidgetItem(julyTr("ORDER_STATE_"+dataList.at(2).toUpper(),dataList.at(2))));postWorkAtTableItem(ui.ordersTable->item(curRow,2));
	ui.ordersTable->setItem(curRow,3,new QTableWidgetItem(currencyASign+" "+dataList.at(3)));ui.ordersTable->item(curRow,3)->setTextAlignment(Qt::AlignVCenter|Qt::AlignRight);
	ui.ordersTable->setItem(curRow,4,new QTableWidgetItem(orderSign+" "+QString::number(dataList.at(4).toDouble(),'f',priceDecimals)));ui.ordersTable->item(curRow,4)->setTextAlignment(Qt::AlignVCenter|Qt::AlignRight);
	ui.ordersTable->setItem(curRow,5,new QTableWidgetItem(orderSign+" "+QString::number(dataList.at(3).toDouble()*dataList.at(4).toDouble(),'f',usdDecimals)));ui.ordersTable->item(curRow,5)->setTextAlignment(Qt::AlignVCenter|Qt::AlignRight);

	setOrdersTableRowStateByText(curRow,dataList.at(2).toAscii());
	ordersSelectionChanged();
	ui.ordersTableFrame->setVisible(true);
	ui.noOpenedOrdersLabel->setVisible(false);
}

void QtBitcoinTrader::setOrdersTableRowStateByText(int row, QByteArray text)
{
	if(text=="invalid")setOrdersTableRowState(row,4);
	else 
	if(text.contains("pending"))setOrdersTableRowState(row,1);
	else
	setOrdersTableRowState(row,2);
}

void QtBitcoinTrader::setOrdersTableRowState(int row, int state)
{
	QColor nColor(255,255,255);
	switch(state)
	{
		case 0: nColor=Qt::lightGray; break;
		case 1: nColor.setRgb(255,255,200); break; //Yellow
		case 2: nColor.setRgb(200,255,200); break; //Green
		case 3: nColor.setRgb(255,200,200); break; //Red
		case 4: nColor.setRgb(200,200,255); break; //Blue
		default: break;
	}
	for(int n=0;n<ui.ordersTable->columnCount();n++)ui.ordersTable->item(row,n)->setBackgroundColor(nColor);
}

void QtBitcoinTrader::setRuleStateBuGuid(quint64 guid, int state)
{
	for(int n=0;n<ui.rulesTable->rowCount();n++)
		if(ui.rulesTable->item(n,0)->data(Qt::UserRole).toUInt()==guid)
		{
			setRulesTableRowState(n,state);
			return;
		}
}

void QtBitcoinTrader::setRulesTableRowState(int row, int state)
{
	QColor nColor(255,255,255);
	QString nText;
	bool pending=false;
	switch(state)
	{
	case 0: nColor=Qt::lightGray; break;
	case 1: nColor.setRgb(255,255,200); pending=true; nText=julyTr("RULE_STATE_PENDING","pending"); break; //Yellow
	case 2: nColor.setRgb(200,255,200); nText=julyTr("RULE_STATE_DONE","done"); break; //Green
	case 3: nColor.setRgb(255,200,200); nText=julyTr("RULE_STATE_DISABLED","disabled");break; //Red
	case 4: nColor.setRgb(200,200,255); break; //Blue
	default: break;
	}
	ui.rulesTable->item(row,0)->setText(nText);
	ui.rulesTable->item(row,1)->setData(Qt::UserRole,pending);
	for(int n=0;n<ui.rulesTable->columnCount();n++)ui.rulesTable->item(row,n)->setBackgroundColor(nColor);
	
	cacheFirstRowGuid();
}

void QtBitcoinTrader::ordersCancelAll()
{
	for(int n=0;n<ui.ordersTable->rowCount();n++)
		if(ui.ordersTable->item(n,2)->data(Qt::UserRole).toString()!="canceled")
			emit cancelOrderByOid(ui.ordersTable->item(n,0)->data(Qt::UserRole).toByteArray());
}

void QtBitcoinTrader::ordersCancelSelected()
{
	for(int n=0;n<ui.ordersTable->rowCount();n++)
		if(ui.ordersTable->item(n,2)->isSelected()&&ui.ordersTable->item(n,2)->data(Qt::UserRole).toString()!="canceled")
			emit cancelOrderByOid(ui.ordersTable->item(n,0)->data(Qt::UserRole).toByteArray());
}

void QtBitcoinTrader::calcButtonClicked()
{
	(new FeeCalculator)->show();
}

void QtBitcoinTrader::checkValidSellButtons()
{
	ui.sellBitcoinsButton->setEnabled(ui.sellTotalBtc->value()>=0.01&&ui.sellTotalBtc->value()<=ui.accountBTC->value());
}

void QtBitcoinTrader::sellPricePerCoinAsMarketPrice()
{
	ui.sellPricePerCoin->setValue(ui.marketBuy->value());
}

void QtBitcoinTrader::sellTotalBtcToSellAllIn()
{
	ui.sellTotalBtc->setValue(ui.accountBTC->value());
}

void QtBitcoinTrader::sellTotalBtcToSellHalfIn()
{
	ui.sellTotalBtc->setValue(ui.accountBTC->value()/2.0);
}

void QtBitcoinTrader::lastSoftLagChanged(double)
{
	bool lastL=lastLagState;
	if(!isValidSoftLag())
	{
		lastLagState=false;
		ui.lastUpdate->setStyleSheet("QDoubleSpinBox {background: #ffaaaa;}");
	}
	else
	{
		ui.lastUpdate->setStyleSheet("");
		lastLagState=true;
	}
	if(lastL!=lastLagState)
	{
		checkValidSellButtons();
		checkValidBuyButtons();
		ui.ordersControls->setEnabled(lastLagState);
		ui.buyButtonBack->setEnabled(lastLagState);
		ui.sellButtonBack->setEnabled(lastLagState);
		QString toolTip;
		if(!lastLagState)toolTip=julyTr("TOOLTIP_API_LAG_TO_HIGH","API Lag is to High");

		ui.ordersControls->setToolTip(toolTip);
		ui.buyButtonBack->setToolTip(toolTip);
		ui.sellButtonBack->setToolTip(toolTip);

		if(lastLagState)checkAllRules();
	}
}

void QtBitcoinTrader::checkAllRules()
{
	marketBuyChanged(ui.marketBuy->value());
	marketSellChanged(ui.marketSell->value());
	marketLowChanged(ui.marketLow->value());
	marketHighChanged(ui.marketHigh->value());
	marketLastChanged(ui.marketLast->value());
	ordersLastBuyPriceChanged(ui.ordersLastBuyPrice->value());
	ordersLastSellPriceChanged(ui.ordersLastSellPrice->value());
}

void QtBitcoinTrader::sellTotalBtcToSellChanged(double val)
{
	profitSellThanBuyCalc();
	if(sellLockBtcToSell)return;
	sellLockBtcToSell=true;

	sellLockAmountToReceive=true;
	ui.sellAmountToReceive->setValue(ui.sellPricePerCoin->value()*val*floatFeeDec);

	ui.sellThenBuyGroupBox->setEnabled(val>0.0);
	sellLockAmountToReceive=false;

	checkValidSellButtons();
	sellLockBtcToSell=false;
}

void QtBitcoinTrader::sellPricePerCoinInUsdChanged(double)
{
	profitSellThanBuyCalc();
	if(!sellLockPricePerCoin)
	{
	sellLockPricePerCoin=true;
	sellTotalBtcToSellChanged(ui.sellTotalBtc->value());
	sellLockPricePerCoin=false;
	}
	ui.sellNextMaxBuyPrice->setValue(ui.sellPricePerCoin->value()*floatFeeDec*floatFeeDec);
	ui.sellNextMaxBuyStep->setValue(ui.sellPricePerCoin->value()-ui.sellNextMaxBuyPrice->value());
	checkValidSellButtons();
}

void QtBitcoinTrader::sellAmountToReceiveChanged(double val)
{
	profitBuyThanSellCalc();
	profitSellThanBuyCalc();
	if(sellLockAmountToReceive)return;
	sellLockAmountToReceive=true;

	sellLockBtcToSell=true;
	sellLockPricePerCoin=true;

	ui.sellTotalBtc->setValue(val/ui.sellPricePerCoin->value()/floatFeeDec);

	sellLockPricePerCoin=false;
	sellLockBtcToSell=false;

	sellLockAmountToReceive=false;
	checkValidSellButtons();
}

void QtBitcoinTrader::sellBitcoinButton()
{
	checkValidSellButtons();
	if(ui.sellBitcoinsButton->isEnabled()==false)return;

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setWindowTitle(julyTr("MESSAGE_CONFIRM_SELL_TRANSACTION","Please confirm transaction"));
	msgBox.setText(julyTr("MESSAGE_CONFIRM_SELL_TRANSACTION_TEXT","Are you sure to sell %1 at %2 ?<br><br>Note: If total orders amount of your Bitcoins exceeds your balance, %3 will remove this order immediately.").arg(currencyASign+" "+ui.sellTotalBtc->text()).arg(currencyBSign+" "+ui.sellPricePerCoin->text()).arg(exchangeName));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	if(msgBox.exec()!=QMessageBox::Yes)return;

	emit apiSell(ui.sellTotalBtc->value(),ui.sellPricePerCoin->value());
}

void QtBitcoinTrader::buyTotalToSpendInUsdChanged(double val)
{
	profitBuyThanSellCalc();
	profitSellThanBuyCalc();
	if(buyLockTotalSpend)return;
	buyLockTotalSpend=true;

	buyLockTotalBtc=true;
	ui.buyTotalBtc->setValue(val/ui.buyPricePerCoin->value());
	buyLockTotalBtc=false;

	ui.buyThenSellGroupBox->setEnabled(val>0.0);

	buyLockTotalSpend=false;
	checkValidBuyButtons();
}


void QtBitcoinTrader::buyBtcToBuyChanged(double val)
{
	ui.buyTotalBtcResult->setValue(val*floatFeeDec);
	if(buyLockTotalBtc)
	{
		profitSellThanBuyCalc();
		profitBuyThanSellCalc();
		return;
	}
	buyLockTotalBtc=true;

	buyLockTotalSpend=true;
	ui.buyTotalSpend->setValue(ui.buyTotalBtc->value()*ui.buyPricePerCoin->value());
	buyLockTotalSpend=false;

	buyLockTotalBtc=false;
	profitSellThanBuyCalc();
	profitBuyThanSellCalc();
	checkValidBuyButtons();
}

void QtBitcoinTrader::buyPricePerCoinChanged(double)
{
	if(!buyLockPricePerCoin)
	{
	buyLockPricePerCoin=true;
	buyTotalToSpendInUsdChanged(ui.buyTotalSpend->value());
	buyLockPricePerCoin=false;
	}
	ui.buyNextInSellPrice->setValue(ui.buyPricePerCoin->value()*floatFeeInc*floatFeeInc);
	ui.buyNextMinBuyStep->setValue(ui.buyNextInSellPrice->value()-ui.buyPricePerCoin->value());
	checkValidBuyButtons();
}

void QtBitcoinTrader::checkValidBuyButtons()
{
	ui.buyBitcoinsButton->setEnabled(ui.buyTotalBtc->value()>=0.01&&ui.buyTotalBtc->value()<=ui.accountUSD->value());
}

void QtBitcoinTrader::cacheFirstRowGuid()
{
	for(int n=0;n<ui.rulesTable->rowCount();n++)
	{
		if(ui.rulesTable->item(n,1)->data(Qt::UserRole).toBool())
		{
			firstRowGuid=ui.rulesTable->item(n,0)->data(Qt::UserRole).toUInt();
			return;
		}
	}
	firstRowGuid=-1;
}

void QtBitcoinTrader::checkValidRulesButtons()
{
	int selectedCount=ui.rulesTable->selectedItems().count();
	ui.ruleEditButton->setEnabled(selectedCount==ui.rulesTable->columnCount());
	ui.ruleRemove->setEnabled(selectedCount);
	ui.ruleRemoveAll->setEnabled(ui.rulesTable->rowCount());
	ui.currencyComboBox->setEnabled(ui.rulesTable->rowCount()==0);
	ui.ruleConcurrentMode->setEnabled(ui.rulesTable->rowCount()==0);
	ui.ruleSequencialMode->setEnabled(ui.rulesTable->rowCount()==0);

	ui.rulesNoMessage->setVisible(ui.rulesTable->rowCount()==0);
	ui.rulesTable->setVisible(ui.rulesTable->rowCount());

	ui.ruleUp->setEnabled(ui.ruleEditButton->isEnabled()&&ui.rulesTable->rowCount()>1);
	ui.ruleDown->setEnabled(ui.ruleEditButton->isEnabled()&&ui.rulesTable->rowCount()>1);

	cacheFirstRowGuid();
}

void QtBitcoinTrader::ruleUp()
{
	int curRow=ui.rulesTable->currentRow();
	if(curRow<1)return;

	for(int n=0;n<ui.rulesTable->columnCount();n++)
	{
	QTableWidgetItem *takedItem=ui.rulesTable->takeItem(curRow,n);
	ui.rulesTable->setItem(curRow,n,ui.rulesTable->takeItem(curRow-1,n));
	ui.rulesTable->setItem(curRow-1,n,takedItem);
	}
	ui.rulesTable->selectRow(curRow-1);
	cacheFirstRowGuid();
}

void QtBitcoinTrader::ruleDown()
{
	int curRow=ui.rulesTable->currentRow();
	if(curRow>=ui.rulesTable->rowCount()-1)return;

	for(int n=0;n<ui.rulesTable->columnCount();n++)
	{
		QTableWidgetItem *takedItem=ui.rulesTable->takeItem(curRow,n);
		ui.rulesTable->setItem(curRow,n,ui.rulesTable->takeItem(curRow+1,n));
		ui.rulesTable->setItem(curRow+1,n,takedItem);
	}
	ui.rulesTable->selectRow(curRow+1);
	cacheFirstRowGuid();
}

void QtBitcoinTrader::buyBtcToBuyAllIn()
{
	ui.buyTotalSpend->setValue(ui.accountUSD->value());
}

void QtBitcoinTrader::buyBtcToBuyHalfIn()
{
	ui.buyTotalSpend->setValue(ui.accountUSD->value()/2.0);
}

void QtBitcoinTrader::buyPricePerCoinAsMarketPrice()
{
	ui.buyPricePerCoin->setValue(ui.marketSell->value());
}

void QtBitcoinTrader::ordersSelectionChanged()
{
	ui.ordersCancelAllButton->setEnabled(ui.ordersTable->rowCount());
	ui.ordersSelectNone->setEnabled(ui.ordersTable->selectedItems().count());
	ui.ordersCancelSelected->setEnabled(ui.ordersSelectNone->isEnabled());
}

void QtBitcoinTrader::closeEvent(QCloseEvent *event)
{
	if(ui.rulesTable->rowCount())
	{
	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setWindowTitle("Qt Bitcoin Trader");
	msgBox.setText(julyTr("CONFIRM_EXIT","Are you sure to close Application?<br>Active rules works only while application is running.<br>Note: rules table will be cleared."));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	if(msgBox.exec()!=QMessageBox::Yes){event->ignore();return;}
	}
	QSettings settings(iniFileName,QSettings::IniFormat);

	settings.setValue("TabLogOrdersOnTop",ui.tabOrdersLogOnTop->isChecked());
	settings.setValue("TabRulesOnTop",ui.tabRulesOnTop->isChecked());
	settings.setValue("TabTradesOnTop",ui.tabTradesOnTop->isChecked());
	settings.setValue("TabChartsOnTop",ui.tabChartsOnTop->isChecked());

	settings.setValue("DetachedLog",isDetachedLog);
	settings.setValue("DetachedRules",isDetachedRules);
	settings.setValue("DetachedTrades",isDetachedTrades);
	settings.setValue("DetachedCharts",isDetachedCharts);
	if(isDetachedLog)settings.setValue("DetachedLogGeometry",ui.tabOrdersLog->geometry());
	if(isDetachedRules)settings.setValue("DetachedRulesGeometry",ui.tabRules->geometry());
	if(isDetachedTrades)settings.setValue("DetachedTradesGeometry",ui.tabLastTrades->geometry());
	if(isDetachedCharts)settings.setValue("DetachedChartsGeometry",ui.tabCharts->geometry());
	settings.setValue("TradesCurrentTab",ui.tabWidget->currentIndex());
	settings.sync();
	emit quit();
	event->accept();
}

void QtBitcoinTrader::buyBitcoinsButton()
{
	checkValidBuyButtons();
	if(ui.buyBitcoinsButton->isEnabled()==false)return;

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setWindowTitle(julyTr("MESSAGE_CONFIRM_BUY_TRANSACTION","Please confirm new order"));
	msgBox.setText(julyTr("MESSAGE_CONFIRM_BUY_TRANSACTION_TEXT","Are you sure to buy %1 at %2 ?<br><br>Note: If total orders amount of your funds exceeds your balance, %3 will remove this order immediately.").arg(currencyASign+" "+ui.buyTotalBtc->text()).arg(currencyBSign+" "+ui.buyPricePerCoin->text()).arg(exchangeName));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	if(msgBox.exec()!=QMessageBox::Yes)return;
	
	emit apiBuy(ui.buyTotalBtc->value(),ui.buyPricePerCoin->value());
}

void QtBitcoinTrader::copyDonateButton()
{
	QApplication::clipboard()->setText(ui.bitcoinAddress->text());
	QDesktopServices::openUrl(QUrl("bitcoin:"+ui.bitcoinAddress->text()));
	QMessageBox::information(this,"Qt Bitcoin Trader",julyTr("COPY_DONATE_MESSAGE","Bitcoin address copied to clipboard.<br>Thank you for support!"));
}

void QtBitcoinTrader::postWorkAtTableItem(QTableWidgetItem *item, int align)
{
	switch(align)
	{
	case -1:item->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);break;
	case 0:item->setTextAlignment(Qt::AlignCenter);break;
	case 1:item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);break;
	default: break;
	}
	item->setToolTip(item->text());
}

void QtBitcoinTrader::ruleEditButton()
{
	int curRow=ui.rulesTable->currentRow();
	if(curRow<0)return;

	AddRuleWindow addRule;
	RuleHolder curHolder=getRuleHolderByGuid(ui.rulesTable->item(curRow,0)->data(Qt::UserRole).toUInt());
	if(curHolder.invalidHolder)return;
	addRule.fillByRuleHolder(curHolder);
	if(addRule.exec()!=QDialog::Accepted)return;
	removeRuleByGuid(curHolder.getRuleGuid());
	ui.rulesTable->removeRow(curRow);
	addRuleByHolderInToTable(addRule.getRuleHolder(),curRow);
}

void QtBitcoinTrader::addRuleByHolderInToTable(RuleHolder holder, int preferedRow)
{
	int curRow=preferedRow;
	if(curRow==-1)curRow=ui.rulesTable->rowCount();

	ui.rulesTable->insertRow(curRow);
	ui.rulesTable->setRowHeight(curRow,30);
	ui.rulesTable->setItem(curRow,0,new QTableWidgetItem());postWorkAtTableItem(ui.rulesTable->item(curRow,0));ui.rulesTable->item(curRow,0)->setData(Qt::UserRole,holder.getRuleGuid());
	ui.rulesTable->setItem(curRow,1,new QTableWidgetItem(holder.getDescriptionString()));postWorkAtTableItem(ui.rulesTable->item(curRow,1));
	ui.rulesTable->setItem(curRow,2,new QTableWidgetItem(holder.getSellOrBuyString()));postWorkAtTableItem(ui.rulesTable->item(curRow,2));
	ui.rulesTable->setItem(curRow,3,new QTableWidgetItem(holder.getBitcoinsString()));postWorkAtTableItem(ui.rulesTable->item(curRow,3));
	ui.rulesTable->setItem(curRow,4,new QTableWidgetItem(holder.getPriceText()));postWorkAtTableItem(ui.rulesTable->item(curRow,4));
	setRulesTableRowState(curRow,1);
	switch(holder.getRulePriceType())
	{
	case 1: rulesLastPrice<<holder; break;
	case 2: rulesMarketBuyPrice<<holder; break;
	case 3: rulesMarketSellPrice<<holder; break;
	case 4: rulesMarketHighPrice<<holder; break;
	case 5: rulesMarketLowPrice<<holder; break;
	case 6: rulesOrdersLastBuyPrice<<holder; break;
	case 7: rulesOrdersLastSellPrice<<holder; break;
	default: break;
	}
	ui.rulesTable->selectRow(curRow);

	cacheFirstRowGuid();
}

void QtBitcoinTrader::ruleAddButton()
{
	AddRuleWindow addRule;
	if(addRule.exec()!=QDialog::Accepted)return;

	addRuleByHolderInToTable(addRule.getRuleHolder());

	ui.rulesNoMessage->setVisible(false);
	ui.rulesTable->setVisible(true);
	checkValidRulesButtons();
	checkAllRules();
}

void QtBitcoinTrader::ruleRemove()
{
	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setWindowTitle("Qt Bitcoin Trader");
	msgBox.setText(julyTr("RULE_CONFIRM_REMOVE","Are you sure to remove this rule?"));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	if(msgBox.exec()!=QMessageBox::Yes)return;

	for(int n=ui.rulesTable->rowCount()-1;n>=0;n--)
		if(ui.rulesTable->item(n,0)->isSelected())
		{
			removeRuleByGuid(ui.rulesTable->item(n,0)->data(Qt::UserRole).toUInt());
			ui.rulesTable->removeRow(n--);
		}
	checkValidRulesButtons();
}

bool QtBitcoinTrader::fillHolderByFindedGuid(QList<RuleHolder>*holdersList, RuleHolder *holder, uint guid)
{
	for(int n=0;n<holdersList->count();n++)if((*holdersList)[n].getRuleGuid()==guid){(*holder)=holdersList->at(n);return true;}
	return false;
}

RuleHolder QtBitcoinTrader::getRuleHolderByGuid(uint guid)
{
	RuleHolder findedHolder;
	if(!fillHolderByFindedGuid(&rulesLastPrice,&findedHolder,guid))
	if(!fillHolderByFindedGuid(&rulesMarketBuyPrice,&findedHolder,guid))
	if(!fillHolderByFindedGuid(&rulesMarketSellPrice,&findedHolder,guid))
	if(!fillHolderByFindedGuid(&rulesMarketHighPrice,&findedHolder,guid))
	if(!fillHolderByFindedGuid(&rulesMarketLowPrice,&findedHolder,guid))
	if(!fillHolderByFindedGuid(&rulesOrdersLastBuyPrice,&findedHolder,guid))
	fillHolderByFindedGuid(&rulesOrdersLastSellPrice,&findedHolder,guid);
	return findedHolder;
}

void QtBitcoinTrader::removeRuleByGuid(uint guid)
{
	if(removeRuleByGuidInRuleHolderList(guid,&rulesLastPrice))return;
	if(removeRuleByGuidInRuleHolderList(guid,&rulesMarketBuyPrice))return;
	if(removeRuleByGuidInRuleHolderList(guid,&rulesMarketSellPrice))return;
	if(removeRuleByGuidInRuleHolderList(guid,&rulesMarketHighPrice))return;
	if(removeRuleByGuidInRuleHolderList(guid,&rulesMarketLowPrice))return;
	if(removeRuleByGuidInRuleHolderList(guid,&rulesOrdersLastBuyPrice))return;
	removeRuleByGuidInRuleHolderList(guid,&rulesOrdersLastSellPrice);
}

bool QtBitcoinTrader::removeRuleByGuidInRuleHolderList(uint guid, QList<RuleHolder> *ruleHolderList)
{
	for(int n=0;n<ruleHolderList->count();n++)
		if((*ruleHolderList)[n].getRuleGuid()==guid)
		{
			ruleHolderList->removeAt(n);
			return true;
		}
	return false;
}

void QtBitcoinTrader::ruleRemoveAll()
{
	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setWindowTitle("Qt Bitcoin Trader");
	msgBox.setText(julyTr("RULE_CONFIRM_REMOVE_ALL","Are you sure to remove all rules?"));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	if(msgBox.exec()!=QMessageBox::Yes)return;

	for(int n=0;n<ui.rulesTable->rowCount();n++)
		removeRuleByGuid(ui.rulesTable->item(n,0)->data(Qt::UserRole).toUInt());
	ui.rulesTable->clearContents();
	ui.rulesTable->setRowCount(0);
	checkValidRulesButtons();
}

void QtBitcoinTrader::beep()
{
#ifdef USE_QTMULTIMEDIA
	static AudioPlayer *player=new AudioPlayer(this);
	player->beep();
#endif
}

void QtBitcoinTrader::accountUSDChanged(double)
{
	if(ui.accountUSDBeep->isChecked())beep();
}

void QtBitcoinTrader::accountBTCChanged(double)
{
	if(ui.accountBTCBeep->isChecked())beep();
}

void QtBitcoinTrader::marketLowChanged(double val)
{
	checkAndExecuteRule(&rulesMarketLowPrice,val);
	if(ui.marketLowBeep->isChecked()&&lastMarketLowPrice!=val)
	{
		lastMarketLowPrice=val;
		beep();
	}
}

void QtBitcoinTrader::marketHighChanged(double val)
{
	checkAndExecuteRule(&rulesMarketHighPrice,val);
	if(ui.marketHighBeep->isChecked()&&lastMarketHighPrice!=val)
	{
		lastMarketHighPrice=val;
		beep();
	}
}

void QtBitcoinTrader::marketBuyChanged(double val)			{checkAndExecuteRule(&rulesMarketBuyPrice,val);}
void QtBitcoinTrader::marketSellChanged(double val)			{checkAndExecuteRule(&rulesMarketSellPrice,val);}
void QtBitcoinTrader::marketLastChanged(double val)			{checkAndExecuteRule(&rulesLastPrice,val);}
void QtBitcoinTrader::ordersLastBuyPriceChanged(double val)	{checkAndExecuteRule(&rulesOrdersLastBuyPrice,val);}
void QtBitcoinTrader::ordersLastSellPriceChanged(double val){checkAndExecuteRule(&rulesOrdersLastSellPrice,val);}

void QtBitcoinTrader::checkAndExecuteRule(QList<RuleHolder> *ruleHolder, double price)
{
	for(int n=0;n<ruleHolder->count();n++)
		if((*ruleHolder)[n].isAchieved(price))
		{
			if(!isValidSoftLag()){(*ruleHolder)[n].startWaitingLowLag();continue;}

			uint ruleGuid=(*ruleHolder)[n].getRuleGuid();
			if(firstRowGuid==ruleGuid&&ui.ruleSequencialMode->isChecked()||ui.ruleConcurrentMode->isChecked())
			{
				double ruleBtc=(*ruleHolder)[n].getRuleBtc();
				bool isBuying=(*ruleHolder)[n].isBuying();
				double priceToExec=(*ruleHolder)[n].getRulePrice();
				ruleHolder->removeAt(n--);

				setRuleStateBuGuid(ruleGuid,2);
				if(ruleBtc<0)
				{
					if(ruleBtc==-1.0)ruleBtc=ui.accountBTC->value();
					if(ruleBtc==-2.0)ruleBtc=ui.accountBTC->value()/2.0;
					if(ruleBtc==-3.0)ruleBtc=ui.buyTotalSpend->value()/ui.buyPricePerCoin->value();
					if(ruleBtc==-4.0)ruleBtc=ui.buyTotalSpend->value()/ui.buyPricePerCoin->value()/2.0;
					if(ruleBtc==-5.0){ordersCancelAll();if(ui.ruleBeep->isChecked())beep();continue;}
				}

				if(priceToExec<0)
				{
					if(priceToExec==-1.0)priceToExec=ui.marketLast->value();
					if(priceToExec==-2.0)priceToExec=ui.marketBuy->value();
					if(priceToExec==-3.0)priceToExec=ui.marketSell->value();
					if(priceToExec==-4.0)priceToExec=ui.marketHigh->value();
					if(priceToExec==-5.0)priceToExec=ui.marketLow->value();
					if(priceToExec==-6.0)priceToExec=ui.ordersLastBuyPrice->value();
					if(priceToExec==-7.0)priceToExec=ui.ordersLastSellPrice->value();
					if(priceToExec==-8.0)priceToExec=priceToExec;
				}			

				if(ruleBtc>=0.01)
				{
				if(isBuying)emit apiBuy(ruleBtc,priceToExec);
				else emit apiSell(ruleBtc,priceToExec);
				}
				if(ui.ruleBeep->isChecked())beep();
			}
		}
}

void QtBitcoinTrader::checkIsTabWidgetVisible()
{
	bool isTabWidgetVisible=ui.tabWidget->count();
	static bool lastTabWidgetVisible=isTabWidgetVisible;
	if(isTabWidgetVisible!=lastTabWidgetVisible)
	{
	ui.tabWidget->setVisible(isTabWidgetVisible);
	ui.centerLayout->setHorizontalSpacing(isTabWidgetVisible?6:0);
	ui.groupOrders->setMaximumWidth(isTabWidgetVisible?600:16777215);
	lastTabWidgetVisible=isTabWidgetVisible;
	}
	fixWindowMinimumSize();
}

void QtBitcoinTrader::detachLog()
{
	ui.tabOrdersLog->setParent(0);
	ui.tabOrdersLog->move(mapToGlobal(ui.tabWidget->geometry().topLeft()));
	ui.detachOrdersLog->setVisible(false);
	ui.tabOrdersLogOnTop->setVisible(true);

	QSettings settings(iniFileName,QSettings::IniFormat);
	QRect newGeometry=settings.value("DetachedLogGeometry",ui.tabOrdersLog->geometry()).toRect();
	if(isValidGeometry(&newGeometry))ui.tabOrdersLog->setGeometry(newGeometry);

	ui.tabOrdersLogOnTop->setChecked(settings.value("TabOrdersOnTop",false).toBool());
	isDetachedLog=true;
	tabLogOrdersOnTop(ui.tabOrdersLogOnTop->isChecked());
	checkIsTabWidgetVisible();
}

void QtBitcoinTrader::detachRules()
{
	ui.tabRules->setParent(0);
	ui.tabRules->move(mapToGlobal(ui.tabWidget->geometry().topLeft()));
	ui.detachRules->setVisible(false);
	ui.tabRulesOnTop->setVisible(true);

	QSettings settings(iniFileName,QSettings::IniFormat);
	QRect newGeometry=settings.value("DetachedRulesGeometry",ui.tabRules->geometry()).toRect();
	if(isValidGeometry(&newGeometry))ui.tabRules->setGeometry(newGeometry);

	ui.tabRulesOnTop->setChecked(settings.value("TabRulesOnTop",false).toBool());
	isDetachedRules=true;
	tabRulesOnTop(ui.tabRulesOnTop->isChecked());
	checkIsTabWidgetVisible();
}

void QtBitcoinTrader::detachTrades()
{
	ui.tabLastTrades->setParent(0);
	ui.tabLastTrades->move(mapToGlobal(ui.tabWidget->geometry().topLeft()));
	ui.detachTrades->setVisible(false);
	ui.tabTradesOnTop->setVisible(true);

	QSettings settings(iniFileName,QSettings::IniFormat);
	QRect newGeometry=settings.value("DetachedTradesGeometry",ui.tabLastTrades->geometry()).toRect();
	if(isValidGeometry(&newGeometry))ui.tabLastTrades->setGeometry(newGeometry);

	ui.tabTradesOnTop->setChecked(settings.value("TabTradesOnTop",false).toBool());
	isDetachedTrades=true;
	tabTradesOnTop(ui.tabTradesOnTop->isChecked());
	checkIsTabWidgetVisible();
}

void QtBitcoinTrader::detachCharts()
{
	ui.tabCharts->setParent(0);
	ui.tabCharts->move(mapToGlobal(ui.tabWidget->geometry().topLeft()));
	ui.detachCharts->setVisible(false);
	ui.tabChartsOnTop->setVisible(true);

	QSettings settings(iniFileName,QSettings::IniFormat);
	QRect newGeometry=settings.value("DetachedChartsGeometry",ui.tabCharts->geometry()).toRect();
	if(isValidGeometry(&newGeometry))ui.tabCharts->setGeometry(newGeometry);

	ui.tabChartsOnTop->setChecked(settings.value("TabChartsOnTop",false).toBool());
	isDetachedCharts=true;
	tabChartsOnTop(ui.tabChartsOnTop->isChecked());
	checkIsTabWidgetVisible();
}

void QtBitcoinTrader::attachLog()
{
	ui.tabOrdersLogOnTop->setVisible(false);
	ui.detachOrdersLog->setVisible(true);
	ui.tabWidget->insertTab(0,ui.tabOrdersLog,ui.tabOrdersLog->accessibleName());
	checkIsTabWidgetVisible();
	ui.tabWidget->setCurrentWidget(ui.tabOrdersLog);
	isDetachedLog=false;
}

void QtBitcoinTrader::attachRules()
{
	ui.tabRulesOnTop->setVisible(false);
	ui.detachRules->setVisible(true);
	ui.tabWidget->insertTab(isDetachedLog?0:1,ui.tabRules,ui.tabRules->accessibleName());
	checkIsTabWidgetVisible();
	ui.tabWidget->setCurrentWidget(ui.tabRules);
	isDetachedRules=false;
}

void QtBitcoinTrader::attachTrades()
{
	ui.tabTradesOnTop->setVisible(false);
	ui.detachTrades->setVisible(true);

	int newTabPos=2;
	if(isDetachedLog&&isDetachedRules)newTabPos=0;
	else if(isDetachedLog||isDetachedRules)newTabPos=1;

	ui.tabWidget->insertTab(newTabPos,ui.tabLastTrades,ui.tabLastTrades->accessibleName());
	checkIsTabWidgetVisible();
	ui.tabWidget->setCurrentWidget(ui.tabLastTrades);
	isDetachedTrades=false;
}

void QtBitcoinTrader::attachCharts()
{
	ui.tabChartsOnTop->setVisible(false);
	ui.detachCharts->setVisible(true);
	int newTabPos=3;
	if(isDetachedLog&&isDetachedRules&&isDetachedTrades)newTabPos=0;
	else if(isDetachedLog&&isDetachedRules||isDetachedRules&&isDetachedTrades||isDetachedLog&&isDetachedTrades)newTabPos=1;
	else if(isDetachedLog||isDetachedRules||isDetachedTrades)newTabPos=2;
	ui.tabWidget->insertTab(newTabPos,ui.tabCharts,ui.tabCharts->accessibleName());
	checkIsTabWidgetVisible();
	ui.tabWidget->setCurrentWidget(ui.tabCharts);
	isDetachedCharts=false;
}

bool QtBitcoinTrader::isValidSoftLag()
{
	return ui.lastUpdate->value()<2.0;
}

void QtBitcoinTrader::aboutTranslationButton()
{
	(new TranslationAbout(this))->show();
}

void QtBitcoinTrader::languageChanged()
{
	if(!constructorFinished)return;
	julyTranslator->translateUi(this);
	localDateTimeFormat=julyTr("DATETIME_FORMAT",localDateTimeFormat);
	localTimeFormat=julyTr("TIME_FORMAT",localTimeFormat);
	QStringList ordersLabels;
	ordersLabels<<julyTr("ORDERS_DATE","Date")<<julyTr("ORDERS_TYPE","Type")<<julyTr("ORDERS_STATUS","Status")<<julyTr("ORDERS_AMOUNT","Amount")<<julyTr("ORDERS_PRICE","Price")<<julyTr("ORDERS_TOTAL","Total");
	ui.ordersTable->setHorizontalHeaderLabels(ordersLabels);
	QStringList rulesLabels;
	rulesLabels<<julyTr("RULES_T_STATE","State")<<julyTr("RULES_T_DESCR","Description")<<julyTr("RULES_T_ACTION","Action")<<julyTr("RULES_T_AMOUNT","Amount")<<julyTr("RULES_T_PRICE","Price");
	ui.rulesTable->setHorizontalHeaderLabels(rulesLabels);

	QStringList tradesLabels;
	tradesLabels<<julyTr("ORDERS_AMOUNT","Amount")<<julyTr("ORDERS_DATE","Date")<<julyTr("ORDERS_PRICE","Price")<<"";
	ui.tableTrades->setHorizontalHeaderLabels(tradesLabels);

	ui.tabOrdersLog->setAccessibleName(julyTr("TAB_ORDERS_LOG","Orders Log"));
	ui.tabOrdersLog->setWindowTitle(ui.tabOrdersLog->accessibleName()+" ["+profileName+"]");
	if(isDetachedLog){julyTranslator->translateUi(ui.tabOrdersLog);fixAllChildButtonsAndLabels(ui.tabOrdersLog);}

	ui.tabRules->setAccessibleName(julyTr("TAB_RULES_FOR_ORDERS","Rules for creating Orders"));
	ui.tabRules->setWindowTitle(ui.tabRules->accessibleName()+" ["+profileName+"]");
	if(isDetachedRules){julyTranslator->translateUi(ui.tabRules);fixAllChildButtonsAndLabels(ui.tabRules);}

	ui.tabLastTrades->setAccessibleName(julyTr("TAB_LAST_TRADES","Last Trades"));
	ui.tabLastTrades->setWindowTitle(ui.tabLastTrades->accessibleName()+" ["+profileName+"]");
	if(isDetachedTrades){julyTranslator->translateUi(ui.tabLastTrades);fixAllChildButtonsAndLabels(ui.tabLastTrades);}

	ui.tabCharts->setAccessibleName(julyTr("TAB_CHARTS","Charts"));
	ui.tabCharts->setWindowTitle(ui.tabCharts->accessibleName()+" ["+profileName+"]");
	if(isDetachedCharts){julyTranslator->translateUi(ui.tabCharts);fixAllChildButtonsAndLabels(ui.tabCharts);}

	ui.groupBoxAccount->setTitle(julyTr("ACCOUNT_GROUPBOX","%1 Account").arg(exchangeName));

	for(int n=0;n<ui.tabWidget->count();n++)
		ui.tabWidget->setTabText(n,ui.tabWidget->widget(n)->accessibleName());

	QString curCurrencyName=currencyNamesMap->value(currencyAStr,"BITCOINS");
	ui.buyGroupbox->setTitle(julyTr("GROUPBOX_BUY","Buy %1").arg(curCurrencyName));
	ui.sellGroupBox->setTitle(julyTr("GROUPBOX_SELL","Sell %1").arg(curCurrencyName));

	ui.tableTrades->clearContents();
	ui.tableTrades->setRowCount(0);
	ui.tradesVolume5m->setValue(0.0);

	fixAllChildButtonsAndLabels(this);
	forcedReloadOrders=true;
	emit clearValues();
	emit getHistory(true);
	//emit reloadOrders();
}

void QtBitcoinTrader::buttonNewWindow()
{
	QProcess::startDetached(QApplication::applicationFilePath());
}

bool QtBitcoinTrader::eventFilter(QObject *obj, QEvent *event)
{
	if(obj!=this&&event->type()==QEvent::Close)
	{
		if(obj==ui.tabOrdersLog)QTimer::singleShot(50,this,SLOT(attachLog()));
		else
		if(obj==ui.tabRules)QTimer::singleShot(50,this,SLOT(attachRules()));
		else
		if(obj==ui.tabLastTrades)QTimer::singleShot(50,this,SLOT(attachTrades()));
		else
		if(obj==ui.tabCharts)QTimer::singleShot(50,this,SLOT(attachCharts()));
	}
	return QObject::eventFilter(obj, event);
}