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

#ifndef _GC_EquipmentCalculator_h
#define _GC_EquipmentCalculator_h 1

#include "Context.h"
#include "EquipmentItems.h"

#include <chrono>

class EquipmentCalculator;

class EquipCalculationThread : public QThread
{
    public:
        EquipCalculationThread(EquipmentCalculator* equipCalculator) : eqCalculator_(equipCalculator) {}

    protected:

        // recalculate distances
        virtual void run() override;

    private:
        EquipmentCalculator* eqCalculator_;
};

class EquipmentCalculator : public QObject
{
    Q_OBJECT

    public:

        // Singleton pattern
        static EquipmentCalculator& getInstance() {
            static EquipmentCalculator instance;
            return instance;
        }
        ~EquipmentCalculator();

        EquipmentCalculator(EquipmentCalculator const&) = delete;
        void operator=(EquipmentCalculator const&) = delete;

        // Call with the first (boot strap) athlete's context
        void initialise(Context *context);

        // mechanism to disabled calculations during GC shutdown.
        void disableCalculations(bool disableCalculations);

    protected slots:

        // Global context signal handlers
        void configChanged(qint32 cfg);
        void eqRecalculation(const QString& reason);
        void eqItemRecalculation(const QUuid& equipmentRef, const QString& reason);
        void eqRecalculationComplete();

        // Context signal handlers
        void athleteLoadDone(QString folder, Context* context);
        void athleteloadCompleted(QString folder, Context* context);
        void autoImportCompleted();
        void refreshEnd();
        void rideAdded(RideItem* rideI);
        void rideChanged(RideItem* rideI);
        void rideDeleted(RideItem* rideI);

        // Main Window signal handlers
        void openingAthlete(QString athleteName, Context* context);
        void closedAthlete(QString athleteName);

    private:
        EquipmentCalculator();

        friend class ::EquipCalculationThread;

        bool calculationInProgress(const QString& reason);
        void recalculateCache(const QString& reason);
        void checkRide(RideItem* rideItem);
        RideItem* getNextRide();
        void threadCompleted(EquipCalculationThread* thread);

        // Equipment recalculation
        QMutex updateMutex_;
        QVector<EquipCalculationThread*> recalculationThreads_;
        QVector<RideItem*> rideItemList_;

        std::atomic<bool> calculationsDisabled_;
        std::atomic<int> eqCalculationInProgress_;
        QVector<AbstractEqItem*> allAbsEqItems_;

        uint32_t threadsUsed_;
        uint32_t numActivities_;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime_;

        MainWindow* mainWindow_;
};

#endif // _GC_EquipmentCalculator_h
