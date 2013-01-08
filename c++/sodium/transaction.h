/**
 * Copyright (c) 2012, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#ifndef _SODIUM_TRANSACTION_H_
#define _SODIUM_TRANSACTION_H_

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <sodium/unit.h>
#include <pthread.h>
#include <map>
#include <set>
#include <list>

namespace sodium {

    namespace impl {
        struct nodeID {
            nodeID() : id(0) {}
            nodeID(unsigned long long id) : id(id) {}
            unsigned long long id;
            nodeID succ() const { return nodeID(id+1); }
            inline bool operator < (const nodeID& other) const { return id < other.id; }
        };
        template <class A>
        struct ordered_value {
            ordered_value() : tid(-1) {}
            long long tid;
            boost::optional<A> oa;
        };

        struct ID {
            ID() : id(0) {}
            ID(unsigned id) : id(id) {}
            unsigned id;
            ID succ() const { return ID(id+1); }
            inline bool operator < (const ID& other) const { return id < other.id; }
        };

        struct entryID {
            entryID() : id(0) {}
            entryID(unsigned long long id) : id(id) {}
            unsigned long long id;
            entryID succ() const { return entryID(id+1); }
            inline bool operator < (const entryID& other) const { return id < other.id; }
        };

        class node
        {
            public:
                struct target {
                    target(
                        void* handler,
                        const std::shared_ptr<node>& n
                    ) : handler(handler),
                        n(n) {}
                    void* handler;
                    std::shared_ptr<node> n;
                };

            public:
                node() : rank(0) {}

                unsigned long long rank;
                std::list<node::target> targets;
                std::list<light_ptr> firings;

                void link(void* handler, const std::shared_ptr<node>& target);
                bool unlink(void* handler);

            private:
                void ensure_bigger_than(std::set<node*>& visited, unsigned long long limit);
        };

        unsigned long long rankOf(const std::shared_ptr<node>& target);

        class partition_state {
            public:
                pthread_mutex_t transaction_lock;
                pthread_mutex_t listeners_lock;

                partition_state();
                ~partition_state();

                nodeID allocNodeID();

                static const std::shared_ptr<partition_state>& instance(); 

            private:
                nodeID nextnodeID;
                int nextlistenerID;
        };

        struct transaction_impl;
        struct prioritized_entry {
            prioritized_entry(const std::shared_ptr<impl::node>& target,
                              const std::function<void(transaction_impl*)>& action)
                : target(target), action(action)
            {
            }
            std::shared_ptr<node> target;
            std::function<void(transaction_impl*)> action;
        };

        struct transaction_impl {
            transaction_impl();
            ~transaction_impl();
            entryID next_entry_id;
            std::map<entryID, prioritized_entry> entries;
            std::multimap<unsigned long long, entryID> prioritizedQ;
            std::list<std::function<void()>> lastQ;
            std::list<std::function<void()>> postQ;

            void prioritized(const std::shared_ptr<impl::node>& target,
                             const std::function<void(impl::transaction_impl*)>& action);
            void last(const std::function<void()>& action);
            void post(const std::function<void()>& action);

            bool to_regen;
            void check_regen();
        };
    };

    class transaction
    {
        friend class impl::transaction_impl;
        private:
            static impl::transaction_impl* current_transaction;
            impl::transaction_impl* transaction_was;

        public:
            transaction();
            ~transaction();

            impl::transaction_impl* impl() const { return current_transaction; }
    };

    struct cleaner_upper
    {
        std::function<void()> f;

        cleaner_upper(const std::function<void()>& f) : f(f) {}
        ~cleaner_upper() { f(); }
    };
};  // end namespace sodium

#endif

