/*
 * Copyright (c) 2025 Paul Johnson (paulj49457@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "EquipmentCalculator.h"
#include "EquipmentCache.h"
#include "Athlete.h"
#include "AthleteTab.h"
#include "MainWindow.h"
#include "RideImportWizard.h"


EquipmentCalculator::EquipmentCalculator() :
    eqCalculationInProgress_(0), calculationsDisabled_(false)
{
    qInfo() << "EquipmentCalculator created";

    // cannot use athlete specific signals, as there is only one equipment view.
    connect(GlobalContext::context(), &GlobalContext::configChanged, this, &EquipmentCalculator::configChanged);
    connect(GlobalContext::context(), &GlobalContext::eqRecalculation, this, &EquipmentCalculator::eqRecalculation);
    connect(GlobalContext::context(), &GlobalContext::eqItemRecalculation, this, &EquipmentCalculator::eqItemRecalculation);
    connect(GlobalContext::context(), &GlobalContext::eqRecalculationComplete, this, &EquipmentCalculator::eqRecalculationComplete);
}

EquipmentCalculator::~EquipmentCalculator()
{
    qInfo() << "EquipmentCalculator destroyed";
}

void
EquipmentCalculator::disableCalculations(bool disableCalculations)
{
    calculationsDisabled_ = disableCalculations;
    qInfo() << "EquipmentCalculator::disableCalculations: " << (calculationsDisabled_ ? "disabled" : "enabled");
}

void
EquipmentCalculator::initialise(Context *context)
{
    static bool initialised = false;

    if (!initialised) {
        qInfo() << "EquipmentCalculator::initialise";

        initialised = true;
        mainWindow_ = context->mainWindow;

        // register for athlete opening and closing signals
        connect(context->mainWindow, &MainWindow::openingAthlete, this, &EquipmentCalculator::openingAthlete);
        connect(context->mainWindow, &MainWindow::closedAthlete, this, &EquipmentCalculator::closedAthlete);

        // No opening and closing signals for boot strap athlete/context, so register boot strap athlete here
        openingAthlete("bootstrap athlete", context);
    }
}

void
EquipmentCalculator::openingAthlete(QString athleteName, Context* context)
{
    qInfo() << "EquipmentCalculator::openingAthlete - " << athleteName.toStdString().c_str();

    // register signal for when an athlete's activities have been loaded
    connect(context, &Context::loadDone, this, &EquipmentCalculator::athleteLoadDone);
    connect(context, &Context::loadCompleted, this, &EquipmentCalculator::athleteloadCompleted);
    connect(context, &Context::autoImportCompleted, this, &EquipmentCalculator::autoImportCompleted);
    connect(context, &Context::refreshEnd, this, &EquipmentCalculator::refreshEnd);

    // ride specific signals
    connect(context, &Context::rideAdded, this, &EquipmentCalculator::rideAdded);
    connect(context, &Context::rideChanged, this, &EquipmentCalculator::rideChanged); // metadata, etc
    connect(context, &Context::rideDeleted, this, &EquipmentCalculator::rideDeleted);
}

void
EquipmentCalculator::closedAthlete(QString athleteName)
{
    eqRecalculation("closedAthlete: " + athleteName);
}

void
EquipmentCalculator::athleteLoadDone(QString folder, Context* /* context */ )
{
    eqRecalculation("athleteLoadDone: " + folder);
}

void
EquipmentCalculator::athleteloadCompleted(QString folder, Context* /* context */ )
{
    eqRecalculation("athleteloadCompleted: " + folder);
}

void
EquipmentCalculator::autoImportCompleted()
{
    eqRecalculation("autoImportCompleted");
}

void
EquipmentCalculator::refreshEnd()
{
    eqRecalculation("refreshEnd");
}

void
EquipmentCalculator::rideAdded(RideItem* rideI)
{
    if (!rideI->planned) {

        // check whether an auto import is in progress, prevents numerous recalculations,
        // batch import recalculation is triggered by a separate signal at the end of the import.
        for (auto itr = mainWindow_->athleteTabs().keyValueBegin(); itr != mainWindow_->athleteTabs().keyValueEnd(); ++itr) {
            if (itr->second->context->athlete->autoImport) {
                if (itr->second->context->athlete->autoImport->importInProcess() ) {
                    qInfo() << "EquipmentCalculator::rideAdded - import in progress";
                    return;
                }
            }
        }
        eqRecalculation("rideAdded");
    }
}

void
EquipmentCalculator::rideChanged(RideItem* rideI)
{
    if (!rideI->planned) eqRecalculation("rideChanged");
}

void
EquipmentCalculator::rideDeleted(RideItem* rideI)
{
    if (!rideI->planned) eqRecalculation("rideDeleted");
}

void
EquipmentCalculator::configChanged(qint32 cfg)
{
    // Update in case the metric/imperial units have changed
    if (cfg & CONFIG_UNITS) {

        // Need to rescale for the change of units for the user entered data.
        const QMap<QUuid, AbstractEqItem*>& allAbsEqItems = EquipmentCache::getInstance().getMap();

        for (auto itr = allAbsEqItems.keyValueBegin(); itr != allAbsEqItems.keyValueEnd(); ++itr) {
            itr->second->unitsChanged();
        }

        // always recalculate if the units change, otherwise the results will
        // be wrong until another units change event is received.
        eqRecalculation("unitsChanged");
    }
}

bool
EquipmentCalculator::calculationInProgress(const QString& reason)
{
    // are we already recalcuating?
    if (++eqCalculationInProgress_ > 1) {
        qInfo() << "EquipmentCalculator::calculation already in progress, deferred - " << reason.toStdString().c_str();
        return true;
    }
    return false;
}

void
EquipmentCalculator::eqItemRecalculation(const QUuid& equipmentRef, const QString& reason)
{
    if (calculationsDisabled_) return;
    if (calculationInProgress(reason)) return;

    // find cached equipment using the tile's reference from perspective file
    AbstractEqItem* absEqItem = EquipmentCache::getInstance().getEquipment(equipmentRef);

    if (absEqItem) {

        allAbsEqItems_.clear();
        allAbsEqItems_.append(absEqItem);
        recalculateCache(reason);
    } else {
        // calculation aborted, restore calculations in progress counter.
        --eqCalculationInProgress_;
    }
}

void
EquipmentCalculator::eqRecalculation(const QString& reason)
{
    if (calculationsDisabled_) return;
    if (calculationInProgress(reason)) return;

    allAbsEqItems_.clear();

    const QMap<QUuid, AbstractEqItem*>& allAbsEqItems = EquipmentCache::getInstance().getMap();
    for (auto itr = allAbsEqItems.keyValueBegin(); itr != allAbsEqItems.keyValueEnd(); ++itr) {
        allAbsEqItems_.append(itr->second);
    }
    recalculateCache(reason);
}

void
EquipmentCalculator::recalculateCache(const QString& reason)
{
    // Reset all the tile's distances
    for (AbstractEqItem* absEqItem : allAbsEqItems_) {
        absEqItem->startOfCalculation();
    }

    // for each "open" athlete take a copy of their actual rides (ignore planned rides), creating an overall ride list to process
    for (auto itr = mainWindow_->athleteTabs().keyValueBegin(); itr != mainWindow_->athleteTabs().keyValueEnd(); ++itr) {
        for (RideItem* rideI : itr->second->context->athlete->rideCache->rides()) {
            if (!rideI->planned) rideItemList_.push_back(rideI);
        }
    }

    qInfo() << "EquipmentCalculator::Recalculate Cache - " << reason.toStdString().c_str()
            << ", rides:" << rideItemList_.size() << ", tiles:" << allAbsEqItems_.size();

    // empty ride list then no calculation possible, abort calculation
    if (rideItemList_.isEmpty()) {
        
        // calculation aborted, restore calculations in progress counter.
        --eqCalculationInProgress_;
        return;
    }

    numActivities_ = rideItemList_.size();

    startTime_ = std::chrono::high_resolution_clock::now();

    // calculate number of threads and work per thread
    int maxthreads = QThreadPool::globalInstance()->maxThreadCount();
    int threads = maxthreads / 3; // Don't need many threads
    if (threads == 0) threads = 1; // but need at least one!
    threadsUsed_ = threads;

    // keep launching the threads
    while (threads--) {

        EquipCalculationThread* thread = new EquipCalculationThread(this);
        recalculationThreads_ << thread;
        thread->start();
    }
}

RideItem*
EquipmentCalculator::getNextRide()
{
    updateMutex_.lock();
    RideItem*  item = rideItemList_.isEmpty() ? nullptr : rideItemList_.takeLast();
    updateMutex_.unlock();

    return(item);
}

void
EquipCalculationThread::run() {

    RideItem* item = eqCalculator_->getNextRide();
    while (item != nullptr) {
        eqCalculator_->checkRide(item);
        item = eqCalculator_->getNextRide();
    }
    eqCalculator_->threadCompleted(this);
    return;
}

void
EquipmentCalculator::checkRide(RideItem* rideItem)
{
    // get the date of the activity 
    QDate actDate(QDate(1900, 01, 01).addDays(rideItem->getText("Start Date", "0").toInt()));

    double rideDistance = rideItem->getForSymbol("total_distance", GlobalContext::context()->useMetricUnits);
    double rideElevation = rideItem->getForSymbol("elevation_gain", GlobalContext::context()->useMetricUnits);
    uint64_t rideTimeInSecs = static_cast<uint64_t>(rideItem->getForSymbol("time_riding"));

    QStringList rideEqLinkNameList = rideItem->getText("EquipmentLink", "").simplified().remove(' ').split(",");

    for (AbstractEqItem* absEqItem : allAbsEqItems_) {

        absEqItem->addActivity(rideEqLinkNameList, actDate,
                                rideDistance, rideElevation, rideTimeInSecs,
                                rideItem->context->athlete->cyclist);
    }
}

void
EquipmentCalculator::threadCompleted(EquipCalculationThread* thread)
{
    updateMutex_.lock();
    recalculationThreads_.removeOne(thread);
    updateMutex_.unlock();

    // if the final thread is finished, then update the summary items.
    if (recalculationThreads_.count() == 0) {

        auto endTime = std::chrono::high_resolution_clock::now();

        qInfo() << "EquipmentCalculator::Calculation Complete: stats: Threads:" << threadsUsed_
                << ", Tiles:" << allAbsEqItems_.size() << ", Activities:" << numActivities_
                << ", Time:" << std::chrono::duration<double, std::milli>(endTime-startTime_).count() << " mSecs";

        // Update display values for all the tiles
        for (AbstractEqItem* absEqItem : allAbsEqItems_) {
            absEqItem->endOfCalculation();
        }

        --eqCalculationInProgress_;
        GlobalContext::context()->notifyEqRecalculationComplete();
    }
}

void
EquipmentCalculator::eqRecalculationComplete()
{
    // check another request hasn't been made.
    if (eqCalculationInProgress_ > 0) {
        
        eqCalculationInProgress_ = 0;
        eqRecalculation("deferred recalculation");
    }
}
