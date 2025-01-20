// ./FrontEnd/CharacterInfo/scripts/state/classBrowserState.js

class ClassBrowserState {
constructor() {
    this.filters = {
        search: '',
        types: new Set(),
        categoryId: null,
        subcategoryId: null,
        hasPrerequisites: false,
        isRacial: false,
        statRanges: {
            hp: { min: null, max: null },
            mp: { min: null, max: null }
        }
    };

    this.pagination = {
        page: 1,
        pageSize: 20,
        total: 0,
        loading: false
    };

    this.sort = {
        field: 'name',
        direction: 'asc'
    };

    this.listeners = new Set();
}

subscribe(listener) {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
}

notify() {
    this.listeners.forEach(listener => listener(this.getState()));
}

getState() {
    return {
        filters: { ...this.filters },
        pagination: { ...this.pagination },
        sort: { ...this.sort }
    };
}

setSearch(term) {
    this.filters.search = term;
    this.pagination.page = 1;
    this.notify();
}

setFilter(type, value) {
    switch (type) {
        case 'type':
            if (value.active) {
                this.filters.types.add(value.value);
            } else {
                this.filters.types.delete(value.value);
            }
            break;
        case 'category':
            this.filters.categoryId = value;
            this.filters.subcategoryId = null;
            break;
        case 'subcategory':
            this.filters.subcategoryId = value;
            break;
        case 'prerequisites':
            this.filters.hasPrerequisites = value;
            break;
        case 'racial':
            this.filters.isRacial = value;
            break;
        case 'stat_range':
            const { stat, bound, value: rangeValue } = value;
            this.filters.statRanges[stat][bound] = rangeValue;
            break;
    }
    this.pagination.page = 1;
    this.notify();
}

resetFilters() {
    this.filters = {
        search: '',
        types: new Set(),
        categoryId: null,
        subcategoryId: null,
        hasPrerequisites: false,
        isRacial: false,
        statRanges: {
            hp: { min: null, max: null },
            mp: { min: null, max: null }
        }
    };
    this.pagination.page = 1;
    this.notify();
}

setSort(field, direction) {
    this.sort.field = field;
    this.sort.direction = direction;
    this.pagination.page = 1;
    this.notify();
}

nextPage() {
    if (!this.pagination.loading && 
        this.pagination.page * this.pagination.pageSize < this.pagination.total) {
        this.pagination.page += 1;
        this.notify();
    }
}

setLoading(loading) {
    this.pagination.loading = loading;
    this.notify();
}

setTotal(total) {
    this.pagination.total = total;
    this.notify();
}
}

// Create singleton instance
export const classBrowserState = new ClassBrowserState();