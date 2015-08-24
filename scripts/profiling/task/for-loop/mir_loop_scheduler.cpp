// Author: Ananya Muddukrishna.
// E-mail: ananya@kth.se.

#include <iostream>
#include <fstream>
#include <assert.h>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

// This definition comes from MIR.
// DO NOT EDIT.
#define MIR_IMPOSSIBLE_CPU_ID 299792458

#define SEARCH_TIMEOUT_MS 20000
#define NUM_BRUTE_SEARCH_STEPS 10

// Choose one optimal CPU assignment from below:
//#define OPTIMAL_CPU_ASSIGNMENT_START_FROM_END
#define OPTIMAL_CPU_ASSIGNMENT_START_ONE_OFF

struct Chunk {
    long chunk_start;
    long chunk_end;
    unsigned int cpu_id;
    unsigned int work_time_us;

    Chunk(long cs, long ce, unsigned int cp, unsigned int wt)
        : chunk_start(cs)
        , chunk_end(ce)
        , cpu_id(cp)
        , work_time_us(wt)
    { /*{{{*/
    } /*}}}*/

    bool operator<(const Chunk& c) const
    { /*{{{*/
        return (work_time_us < c.work_time_us);
    } /*}}}*/

    bool operator>(const Chunk& c) const
    { /*{{{*/
        return (work_time_us > c.work_time_us);
    } /*}}}*/

    void print(std::ostream& o) const
    { /*{{{*/
        o << chunk_start << ","
          << chunk_end << ","
          << cpu_id << ","
          << work_time_us << std::endl;
    } /*}}}*/
};

class Schedule {
private:
    std::vector<Chunk> chunks;
    std::set<unsigned int> cpus;
    unsigned int total_work;
    unsigned int makespan;
    unsigned int trial_bound_makespan;
    unsigned int lower_bound_num_cpus;

    // Greedy upper bound on makespan
    unsigned int get_greedy_upper_bound_makespan() const
    { /*{{{*/
        // Calculate upper bound using a greedy strategy.
        // Take as many bins as number of CPUs.
        // Place each item (chunk) in bin that can hold the item
        // with largest space to spare i.e., largest slack.

        // Allocate bins.
        unsigned int num_bins = get_num_cpus();
        unsigned int* bins = new unsigned int[num_bins];
        for (unsigned int j = 0; j < num_bins; j++)
            bins[j] = 0;

        // Place chunks.
        unsigned int num_items = chunks.size();
        for (unsigned int i = 0; i < num_items; i++) {
            unsigned int largest_slack = 0;
            unsigned int bin_largest_slack = 0;
            for (unsigned int j = 0; j < num_bins; j++) {
                unsigned int slack = get_total_work() -
                                     (bins[j] + get_work(i));
                if (slack > largest_slack) {
                    largest_slack = slack;
                    bin_largest_slack = j;
                }
            }
            bins[bin_largest_slack] += get_work(i);
        }

        // Most loaded bin size is the greedy upper bound.
        unsigned int greedy_upper_bound = 0;
        for (unsigned int j = 0; j < num_bins; j++) {
            if (bins[j] > greedy_upper_bound)
                greedy_upper_bound = bins[j];
        }

        delete[] bins;

        return greedy_upper_bound;
    } /*}}}*/

    // Naive lower bound on makespan
    unsigned int get_naive_lower_bound_makespan() const
    { /*{{{*/
        // Naive lower bound is the largest chunk size
        return chunks[0].work_time_us;
    } /*}}}*/

    // Lower bound on number of CPUs
    /*
     * The lower bound is due to: S. Martello, P. Toth. Lower bounds
     * and reduction procedures for the bin packing problem.
     * Discrete and applied mathematics, 28(1):59-70, 1990.
     */
    unsigned int get_L2_lower_bound_cpus() const
    { /*{{{*/
        const unsigned int c = get_upper_bound_makespan();
        const unsigned int n = get_num_chunks();
        unsigned int l = 0;

        // Items in N1 are from 0 ... n1 - 1
        unsigned int n1 = 0;
        // Items in N2 are from n1 ... n12 - 1, we count elements in N1 and N2
        unsigned int n12 = 0;
        // Items in N3 are from n12 ... n3 - 1
        unsigned int n3 = 0;
        // Free space in N2
        unsigned int f2 = 0;
        // Total size of items in N3
        unsigned int s3 = 0;

        // Initialize n12 and f2
        for (; (n12 < n) && (get_work(n12) > c / 2); n12++)
            f2 += c - get_work(n12);

        // Initialize n3 and s3
        for (n3 = n12; n3 < n; n3++)
            s3 += get_work(n3);

        // Compute lower bounds
        for (unsigned int k = 0; k <= c / 2; k++) {
            // Make N1 larger by adding elements and N2 smaller
            for (; (n1 < n) && (get_work(n1) > c - k); n1++)
                f2 -= c - get_work(n1);
            assert(n1 <= n12);
            // Make N3 smaller by removing elements
            for (; (n3 > 0) && (get_work(n3 - 1) < k) && (n3 > n12); n3--)
                s3 -= get_work(n3 - 1);
            // Overspill
            unsigned int o = (s3 > f2) ? ((s3 - f2 + c - 1) / c) : 0;
            l = std::max(l, n12 + o);
        }
        return l;
    } /*}}}*/

public:
    Schedule(const char* schedule_file_name)
        : total_work(0)
        , makespan(0)
        , lower_bound_num_cpus(1)
    { /*{{{*/
        // Read schedule from file
        std::ifstream schedule_file;
        schedule_file.open(schedule_file_name);
        if (schedule_file.is_open()) {
            // The first line in the file is the header
            std::string header;
            std::getline(schedule_file, header);
            // Ensure header matches
            if (header.compare(get_header()) != 0) {
                std::cerr << "Header mismatch.\n"
                          << "Expected: " << get_header()
                          << "\nIn file: " << header << std::endl;
                exit(1);
            }

            // Read remaining lines
            long cs, ce;
            unsigned int wt;
            unsigned int cp;
            char c1, c2, c3; // comma characters
            while ((schedule_file >> cs >> c1 >> ce >> c2 >> cp >> c3 >> wt)
                && (c1 == ',') && (c2 == ',') && (c3 == ',')) {
                cpus.insert(cp);
                total_work += wt;
                chunks.push_back(Chunk(cs, ce, cp, wt));
            }

            schedule_file.close();
        }
        else {
            std::cerr << "Could not open schedule file: "
                      << schedule_file_name << std::endl;
            exit(1);
        }

        // Set makespan
        // Makespan is the largest amount of work among all CPUs
        std::set<unsigned int>::iterator it;
        for (it = cpus.begin(); it != cpus.end(); it++) {
            unsigned int work = get_cpu_work(*it);
            if (work > makespan)
                makespan = work;
        }

        // Sort chunks in descending order
        std::sort(chunks.begin(), chunks.end(), std::greater<Chunk>());

        // Lower bound for num cpus
        lower_bound_num_cpus = get_L2_lower_bound_cpus();

        // Trial bound makespan
        trial_bound_makespan = get_upper_bound_makespan();
    } /*}}}*/

    std::string get_header() const
    { /*{{{*/
        return "\"chunk_start\",\"chunk_end\",\"cpu_id\",\"work_time_us\"";
    } /*}}}*/

    void print (std::ostream& o) const
    { /*{{{*/
        o << "Schedule:" << std::endl;
        o << get_header() << std::endl;
        for (unsigned int i = 0; i < chunks.size(); i++) {
            chunks[i].print(o);
        }
    } /*}}}*/

    void info(std::ostream& o) const
    { /*{{{*/
        o << "Number of chunks: "
          << get_num_chunks() << std::endl;
        o << "Number of CPUs: "
          << get_num_cpus() << std::endl;
        o << "Total work: "
          << get_total_work() << std::endl;
        o << "Makespan: "
          << get_makespan() << std::endl;
        //o << "Greedy upper bound on makespan: "
        //  << get_greedy_upper_bound_makespan() << std::endl;
        o << "Upper bound on makespan: "
          << get_upper_bound_makespan() << std::endl;
        o << "Lower bound on makespan: "
          << get_lower_bound_makespan() << std::endl;
        o << "Upper bound on number of CPUs: "
          << get_upper_bound_cpus() << std::endl;
        o << "Lower bound on number of CPUs: "
          << get_lower_bound_cpus() << std::endl;
    } /*}}}*/

    unsigned int get_num_cpus() const
    { /*{{{*/
        return cpus.size();
    } /*}}}*/

    unsigned int get_work(size_t i) const
    { /*{{{*/
        return chunks[i].work_time_us;
    } /*}}}*/

    Chunk get_chunk(size_t i) const
    { /*{{{*/
        return chunks[i];
    } /*}}}*/

    unsigned int get_total_work() const
    { /*{{{*/
        return total_work;
    } /*}}}*/

    unsigned int get_cpu_work(unsigned int cpu_id) const
    { /*{{{*/
        unsigned int work = 0;
        for (unsigned int i = 0; i < chunks.size(); i++) {
            if (chunks[i].cpu_id == cpu_id)
                work += chunks[i].work_time_us;
        }
        return work;
    } /*}}}*/

    unsigned int get_makespan() const
    { /*{{{*/
        return makespan;
    } /*}}}*/

    // Upper bound on makespan
    unsigned int get_upper_bound_makespan() const
    { /*{{{*/
        return std::min(get_makespan(), get_greedy_upper_bound_makespan()) + 1;
    } /*}}}*/

    // Lower bound on makespan
    unsigned int get_lower_bound_makespan() const
    { /*{{{*/
        return get_naive_lower_bound_makespan();
    } /*}}}*/

    // Get trial bound on makespan
    unsigned int get_trial_bound_makespan() const
    { /*{{{*/
        return trial_bound_makespan;
    } /*}}}*/

    // Set trial bound on makespan
    void set_trial_bound_makespan(unsigned int bound)
    { /*{{{*/
        trial_bound_makespan = bound;
    } /*}}}*/

    // Upper bound on number of CPUs
    unsigned int get_upper_bound_cpus() const
    { /*{{{*/
        return get_num_cpus();
    } /*}}}*/

    // Lower bound on number of CPUs
    unsigned int get_lower_bound_cpus() const
    { /*{{{*/
        //return 1;
        return lower_bound_num_cpus;
    } /*}}}*/

    // Number of chunks
    unsigned int get_num_chunks() const
    { /*{{{*/
        return chunks.size();
    } /*}}}*/
};

#include <gecode/int.hh>
#include <gecode/search.hh>
#include <gecode/minimodel.hh>

using namespace Gecode;

/** \brief Custom brancher implementing CDBF
 *
 * This class implements complete decreasing best fit branching (CDBF)
 * from: Ian Gent and Toby Walsh. From approximate to optimal solutions:
 * Constructing pruning and propagation rules. IJCAI 1997.
 *
 * Additional domination rules are taken from: Paul Shaw. A Constraint
 * for Bin Packing. CP 2004
 *
 * \relates BinPacking
 */
class CDBF : public Brancher { /*{{{*/
protected:
    /// Views for the loads
    ViewArray<Int::IntView> load;
    /// Views for the bins
    ViewArray<Int::IntView> bin;
    /// Array of sizes (shared)
    IntSharedArray size;
    /// Next view to branch on
    mutable unsigned int item;
    /// %Choice
    class Choice : public Gecode::Choice {
    public:
        /// Item
        unsigned int item;
        /// Bins with same slack
        int* same;
        /// Number of bins with same slack
        unsigned int n_same;
        /** Initialize choice for brancher \a b, alternatives \a a,
     *  item \a i and same bins \a s.
     */
        Choice(const Brancher& b, unsigned int a,
               unsigned int i, int* s, unsigned int n_s)
            : Gecode::Choice(b, a)
            , item(i)
            , same(heap.alloc<int>(n_s))
            , n_same(n_s)
        {
            for (unsigned int k = n_same; k--;)
                same[k] = s[k];
        }
        /// Report size occupied
        virtual size_t size(void) const
        {
            return sizeof(Choice) + sizeof(int) * n_same;
        }
        /// Archive into \a e
        virtual void archive(Archive& e) const
        {
            Gecode::Choice::archive(e);
            e << alternatives() << item << n_same;
            for (unsigned int i = n_same; i--;)
                e << same[i];
        }
        /// Destructor
        virtual ~Choice(void)
        {
            heap.free<int>(same, n_same);
        }
    };

public:
    /// Construct brancher
    CDBF(Home home, ViewArray<Int::IntView>& l, ViewArray<Int::IntView>& b,
        IntSharedArray& s)
        : Brancher(home)
        , load(l)
        , bin(b)
        , size(s)
        , item(0)
    {
        home.notice(*this, AP_DISPOSE);
    }
    /// Brancher post function
    static BrancherHandle post(Home home, ViewArray<Int::IntView>& l,
        ViewArray<Int::IntView>& b,
        IntSharedArray& s)
    {
        return *new (home) CDBF(home, l, b, s);
    }
    /// Copy constructor
    CDBF(Space& home, bool share, CDBF& cdbf)
        : Brancher(home, share, cdbf)
        , item(cdbf.item)
    {
        load.update(home, share, cdbf.load);
        bin.update(home, share, cdbf.bin);
        size.update(home, share, cdbf.size);
    }
    /// Copy brancher
    virtual Actor* copy(Space& home, bool share)
    {
        return new (home) CDBF(home, share, *this);
    }
    /// Delete brancher and return its size
    virtual size_t dispose(Space& home)
    {
        home.ignore(*this, AP_DISPOSE);
        size.~IntSharedArray();
        return sizeof(*this);
    }
    /// Check status of brancher, return true if alternatives left
    virtual bool status(const Space&) const
    {
        for (unsigned int i = item; i < bin.size(); i++)
            if (!bin[i].assigned()) {
                item = i;
                return true;
            }
        return false;
    }
    /// Return choice
    virtual Gecode::Choice* choice(Space& home)
    {
        assert(!bin[item].assigned());

        unsigned int n = bin.size(), m = load.size();

        Region region(home);

        // Free space in bins
        int* free = region.alloc<int>(m);

        for (unsigned int j = m; j--;)
            free[j] = load[j].max();
        for (unsigned int i = n; i--;)
            if (bin[i].assigned())
                free[bin[i].val()] -= size[i];

        // Equivalent bins with same free space
        int* same = region.alloc<int>(m + 1);
        unsigned int n_same = 0;
        unsigned int n_possible = 0;

        // Initialize such that failure is guaranteed (pack into bin -1)
        same[n_same++] = -1;

        // Find a best-fit bin for item
        unsigned int slack = INT_MAX;
        for (Int::ViewValues<Int::IntView> j(bin[item]); j(); ++j)
            if (size[item] <= free[j.val()]) {
                // Item still can fit into the bin
                n_possible++;
                if (free[j.val()] - size[item] < slack) {
                    // A new, better fit
                    slack = free[j.val()] - size[item];
                    same[0] = j.val();
                    n_same = 1;
                }
                else if (free[j.val()] - size[item] == slack) {
                    // An equivalent bin, remember it
                    same[n_same++] = j.val();
                }
            }
        /*
     * Domination rules:
     *  - if the item fits the bin exactly, just assign
     *  - if all possible bins are equivalent, just assign
     *
     * Also catches failure: if no possible bin was found, commit
     * the item into bin -1.
     */
        if ((slack == 0) || (n_same == n_possible) || (slack == INT_MAX))
            return new Choice(*this, 1, item, same, 1);
        else
            return new Choice(*this, 2, item, same, n_same);
    }
    /// Return choice
    virtual const Gecode::Choice* choice(const Space& home, Archive& e)
    {
        unsigned int alt, item, n_same;
        e >> alt >> item >> n_same;
        Region re(home);
        int* same = re.alloc<int>(n_same);
        for (unsigned int i = n_same; i--;)
            e >> same[i];
        return new Choice(*this, alt, item, same, n_same);
    }
    /// Perform commit for choice \a _c and alternative \a a
    virtual ExecStatus commit(Space& home, const Gecode::Choice& _c,
        unsigned int a)
    {
        const Choice& c = static_cast<const Choice&>(_c);
        // This catches also the case that the choice has a single aternative only
        if (a == 0) {
            GECODE_ME_CHECK(bin[c.item].eq(home, c.same[0]));
        }
        else {
            Iter::Values::Array same(c.same, c.n_same);

            GECODE_ME_CHECK(bin[c.item].minus_v(home, same));

            for (unsigned int i = c.item + 1; (i < bin.size()) && (size[i] == size[c.item]); i++) {
                same.reset();
                GECODE_ME_CHECK(bin[i].minus_v(home, same));
            }
        }
        return ES_OK;
    }
    /// Print explanation
    virtual void print(const Space&, const Gecode::Choice& _c,
        unsigned int a,
        std::ostream& o) const
    {
        const Choice& c = static_cast<const Choice&>(_c);
        if (a == 0) {
            o << "bin[" << c.item << "] = " << c.same[0];
        }
        else {
            o << "bin[" << c.item;
            for (unsigned int i = c.item + 1; (i < bin.size()) && (size[i] == size[c.item]); i++)
                o << "," << i;
            o << "] != ";
            for (unsigned int i = 0; i < c.n_same - 1; i++)
                o << c.same[i] << ",";
            o << c.same[c.n_same - 1];
        }
    }
}; /*}}}*/

/// Post branching (assumes that \a s is sorted)
BrancherHandle cdbf(Home home, const IntVarArgs& l, const IntVarArgs& b,
    const IntArgs& s)
{ /*{{{*/
    if (b.size() != s.size())
        throw Int::ArgumentSizeMismatch("cdbf");
    ViewArray<Int::IntView> load(home, l);
    ViewArray<Int::IntView> bin(home, b);
    IntSharedArray size(s);
    return CDBF::post(home, load, bin, size);
} /*}}}*/

/**
 * \brief %Example: Bin packing
 *
 * \ingroup Example
 *
 * Modified by Ananya Muddukrishna.
 * Modifications:
 * - Constructor uses Schedule object.
 * - Is Based on IntMinimizeScript instead of IntMinimizeSpace.
 * - Prints schedule object with optimal CPU assignment.
 * - No other functional changes.
 */
class BinPacking : public IntMinimizeSpace {
protected:
    // Schedule
    Schedule schedule;
    // Load for each bin
    IntVarArray load;
    // Bin for each item
    IntVarArray bin;
    // Number of bins
    IntVar bins;

public:
    /// Actual model
    BinPacking(Schedule& sched)
        : schedule(sched)
        , load(*this, schedule.get_upper_bound_cpus(), 0,
              (schedule.get_trial_bound_makespan() < INT_MAX) ?
               schedule.get_trial_bound_makespan() :
               INT_MAX)
        , bin(*this, schedule.get_num_chunks(), 0,
              schedule.get_upper_bound_cpus() - 1)
        , bins(*this, schedule.get_lower_bound_cpus(),
              schedule.get_upper_bound_cpus())
    { /*{{{*/
        // Number of items
        unsigned int n = bin.size();
        // Number of bins
        unsigned int m = load.size();

        // Size of all items
        unsigned int s = 0;
        for (unsigned int i = 0; i < n; i++)
            s += schedule.get_work(i);

        // Array of sizes
        IntArgs sizes(n);
        for (unsigned int i = 0; i < n; i++)
            sizes[i] = schedule.get_work(i);

#ifdef USE_NAIVE_PROPAGATOR
        // All loads must add up to all item sizes
        linear(*this, load, IRT_EQ, s);

        // Load must be equal to packed items
        BoolVarArgs _x(*this, n * m, 0, 1);
        Matrix<BoolVarArgs> x(_x, n, m);

        for (unsigned int i = 0; i < n; i++)
            channel(*this, x.col(i), bin[i]);

        for (unsigned int j = 0; j < m; j++)
            linear(*this, sizes, x.row(j), IRT_EQ, load[j]);
#else // Use Gecode built-in
        binpacking(*this, load, bin, sizes);
#endif

        // Break symmetries
        for (unsigned int i = 1; i < n; i++)
            if (schedule.get_work(i - 1) == schedule.get_work(i))
                rel(*this, bin[i - 1] <= bin[i]);

        // Pack items that require a bin for sure! (wlog)
        {
            unsigned int i = 0;
            // These items all need a bin due to their own size
            for (; (i < n) && (i < m) && (schedule.get_work(i) * 2 > schedule.get_trial_bound_makespan()); i++)
                rel(*this, bin[i] == i);
            // Check if the next item cannot fit to position i-1
            if ((i < n) && (i < m) && (i > 0) && (schedule.get_work(i - 1) + schedule.get_work(i) > schedule.get_trial_bound_makespan()))
                rel(*this, bin[i] == i);
        }

        // All excess bins must be empty
        for (unsigned int j = schedule.get_lower_bound_cpus() + 1; j <= schedule.get_upper_bound_cpus(); j++)
            rel(*this, (bins < j) == (load[j - 1] == 0));

        branch(*this, bins, INT_VAL_MIN());
#ifdef USE_NAIVE_BRANCHER
        branch(*this, bin, INT_VAR_NONE(), INT_VAL_MIN());
#else // Use CDBF, a special brancher supplied used in bin-packing example.
        cdbf(*this, load, bin, sizes);
#endif
    } /*}}}*/

    // Return cost
    virtual IntVar cost(void) const
    { /*{{{*/
        return bins;
    } /*}}}*/

    // Constructor for cloning \a s
    BinPacking(bool share, BinPacking& s)
        : IntMinimizeSpace(share, s)
        , schedule(s.schedule)
    { /*{{{*/
        load.update(*this, share, s.load);
        bin.update(*this, share, s.bin);
        bins.update(*this, share, s.bins);
    } /*}}}*/

    // Copy during cloning
    virtual Space* copy(bool share)
    { /*{{{*/
        return new BinPacking(share, *this);
    } /*}}}*/

    // Print schedule
    virtual void print_schedule(std::ostream& os) const
    {/*{{{*/
        os << schedule.get_header() << std::endl;
        unsigned int n = bin.size();
        unsigned int m = load.size();
        for (unsigned int j = 0; j < m; j++) {
            for (unsigned int i = 0; i < n; i++)
                if (bin[i].assigned() && (bin[i].val() == j)) {
                    Chunk c = schedule.get_chunk(i);
                    // Assign optimal CPU.
#ifdef OPTIMAL_CPU_ASSIGNMENT_START_FROM_END
                    c.cpu_id = schedule.get_num_cpus() - 1 - j;
#elif defined(OPTIMAL_CPU_ASSIGNMENT_START_ONE_OFF)
                    c.cpu_id = (j + 1) % schedule.get_num_cpus();
#else
                    c.cpu_id = j;
#endif
                    c.print(os);
                }
        }
        if (!bin.assigned()) {
            for (unsigned int i = 0; i < n; i++)
                if (!bin[i].assigned()) {
                    Chunk c = schedule.get_chunk(i);
                    // Assign impossible CPU.
                    // The parser in MIR expects an unsigned int.
                    // So we cannot use NA or -1.
                    c.cpu_id = MIR_IMPOSSIBLE_CPU_ID;
                    c.print(os);
                }
        }
    }/*}}}*/

    // Print solution
    virtual void print(std::ostream& os) const
    { /*{{{*/
        unsigned int n = bin.size();
        unsigned int m = load.size();
        os << "Bins used: " << bins << " (from " << m << " bins)." << std::endl;
        for (unsigned int j = 0; j < m; j++) {
            bool fst = true;
            os << "\t[" << j << "]={";
            for (unsigned int i = 0; i < n; i++)
                if (bin[i].assigned() && (bin[i].val() == j)) {
                    if (fst) {
                        fst = false;
                    }
                    else {
                        os << ",";
                    }
                    os << i;
                }
            os << "} #" << load[j] << std::endl;
        }
        if (!bin.assigned()) {
            os << std::endl
               << "Unpacked items:" << std::endl;
            for (unsigned int i = 0; i < n; i++)
                if (!bin[i].assigned())
                    os << "\t[" << i << "] = " << bin[i] << std::endl;
        }
    } /*}}}*/
};

int main(int argc, char* argv[])
{/*{{{*/
    // Read arguments
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0]
                  << " schedule_file_name" << std::endl;
        exit(1);
    }
    const char* schedule_file_name = argv[1];

    // Parse schedule
    std::cout << "Reading schedule in file: "
              << schedule_file_name << std::endl;
    Schedule s(schedule_file_name);
    std::cout << "Schedule info: " << std::endl;
    s.info(std::cout);

    // Brute force trial
    // TODO: Model this as a binary search intead of search from lowest.
    BinPacking* sol = NULL;
    unsigned int lb = s.get_lower_bound_makespan();
    unsigned int ub = s.get_upper_bound_makespan();
    unsigned int step = (ub - lb)/NUM_BRUTE_SEARCH_STEPS;
    if (step == 0)
        step = 1;
    std::cout << "Searching for bin-optimal schedule within makespan bounds ["
              << lb << " : "
              << ub << ") using " << step << " steps ..." << std::endl;
    for (unsigned int i = lb; i < ub; i+=step ) {
        s.set_trial_bound_makespan(i);

        // Watchdog timer for search
        Search::Options so;
        Search::TimeStop ts(SEARCH_TIMEOUT_MS);
        so.stop = &ts;

        // Instantiate problem as BAB search.
        BinPacking* bp = new BinPacking(s);
        BAB<BinPacking> bp_bab(bp, so);
        delete bp;

        // BAB search.
        while (BinPacking* bp = bp_bab.next()) {
            // Delete previous solution
            delete sol;
            // Save solution
            sol = bp;
        }
        if (sol) {
            std::cout << "Bin-optimal schedule found for makespan bound: "
                      << i << std::endl;
            std::string opt_schedule_file_name = std::string(argv[1]) +
                                                 std::string("_opt");
            std::ofstream opt_schedule_file;
            opt_schedule_file.open(opt_schedule_file_name.c_str());
            if (opt_schedule_file.is_open()) {
                std::cout << "Writing bin-optimal schedule to file: "
                          << opt_schedule_file_name << std::endl;
                sol->print_schedule(opt_schedule_file);

                opt_schedule_file.close();

                // Print optimal schedule info
                Schedule s_opt(opt_schedule_file_name.c_str());
                std::cout << "Bin-optimal schedule info: " << std::endl;
                s_opt.info(std::cout);

                break;
            } else {
                std::cerr << "Could not open output file: "
                          << opt_schedule_file_name << std::endl;
                exit(1);
            }
        }
    }
    if (!sol)
        std::cout << "No schedules were found within makespan bounds!"
                  << std::endl;

    return 0;
}/*}}}*/

