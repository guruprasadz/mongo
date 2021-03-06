/**
*    Copyright (C) 2013 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*    As a special exception, the copyright holders give permission to link the
*    code of portions of this program with the OpenSSL library under certain
*    conditions as described in each individual source file and distribute
*    linked combinations including the program with the OpenSSL library. You
*    must comply with the GNU Affero General Public License in all respects for
*    all of the code used other than as permitted herein. If you modify file(s)
*    with this exception, you may extend this exception to your version of the
*    file(s), but you are not obligated to do so. If you do not wish to do so,
*    delete this exception statement from your version. If you delete this
*    exception statement from all source files in the program, then also delete
*    it in the license file.
*/

#include <set>
#include <vector>

#include "mongo/base/error_codes.h"
#include "mongo/base/status.h"
#include "mongo/db/curop.h"
#include "mongo/db/extsort.h"
#include "mongo/db/index/btree_based_access_method.h"
#include "mongo/db/index/index_access_method.h"
#include "mongo/db/index/index_descriptor.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/structure/btree/btree_interface.h"

namespace mongo {

    class BtreeBasedBulkAccessMethod : public IndexAccessMethod {
    public:
        /**
         * Does not take ownership of any pointers.
         * All pointers must outlive 'this'.
         */
        BtreeBasedBulkAccessMethod(TransactionExperiment* txn,
                                   BtreeBasedAccessMethod* real,
                                   BtreeInterface* interface,
                                   const IndexDescriptor* descriptor,
                                   int numRecords);

        ~BtreeBasedBulkAccessMethod() {}

        virtual Status insert(TransactionExperiment* txn,
                              const BSONObj& obj,
                              const DiskLoc& loc,
                              const InsertDeleteOptions& options,
                              int64_t* numInserted);

        Status commit(std::set<DiskLoc>* dupsToDrop, CurOp* op, bool mayInterrupt);

        // Exposed for testing.
        static ExternalSortComparison* getComparison(int version, const BSONObj& keyPattern);

        //
        // Stuff below here is a no-op of one form or another.
        //

        virtual Status commitBulk(IndexAccessMethod* bulk,
                                  bool mayInterrupt,
                                  std::set<DiskLoc>* dups) {
            verify(this == bulk);
            return Status::OK();
        }

        virtual Status touch(const BSONObj& obj) {
            return _notAllowed();
        }

        virtual Status touch(TransactionExperiment* txn) const {
            return _notAllowed();
        }

        virtual Status validate(int64_t* numKeys) {
            return _notAllowed();
        }

        virtual Status remove(TransactionExperiment* txn,
                              const BSONObj& obj,
                              const DiskLoc& loc,
                              const InsertDeleteOptions& options,
                              int64_t* numDeleted) {
            return _notAllowed();
        }

        virtual Status validateUpdate(const BSONObj& from,
                                      const BSONObj& to,
                                      const DiskLoc& loc,
                                      const InsertDeleteOptions& options,
                                      UpdateTicket* ticket) {
            return _notAllowed();
        }

        virtual Status update(TransactionExperiment* txn,
                              const UpdateTicket& ticket,
                              int64_t* numUpdated) {
            return _notAllowed();
        }

        virtual Status newCursor(IndexCursor **out) const {
            return _notAllowed();
        }

        virtual Status initializeAsEmpty(TransactionExperiment* txn) {
            return _notAllowed();
        }

        virtual IndexAccessMethod* initiateBulk(TransactionExperiment* txn) {
            return NULL;
        }

    private:
        Status _notAllowed() const {
            return Status(ErrorCodes::InternalError, "cannot use bulk for this yet");
        }

        // Not owned here.
        BtreeBasedAccessMethod* _real;

        // Not owned here.
        BtreeInterface* _interface;

        // The external sorter.
        boost::scoped_ptr<BSONObjExternalSorter> _sorter;

        // A comparison object required by the sorter.
        boost::scoped_ptr<ExternalSortComparison> _sortCmp;

        // How many docs are we indexing?
        unsigned long long _docsInserted;

        // And how many keys?
        unsigned long long _keysInserted;

        // Does any document have >1 key?
        bool _isMultiKey;

        TransactionExperiment* _txn;
    };

}  // namespace mongo
