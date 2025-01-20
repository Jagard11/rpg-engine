// ./FrontEnd/CharacterInfo/scripts/services/classService.js

class ClassService {
constructor(state) {
    this.state = state;
    this.controller = null;
}

async fetchClasses() {
    try {
        // Cancel previous request if exists
        if (this.controller) {
            this.controller.abort();
        }
        this.controller = new AbortController();

        const state = this.state.getState();
        const { filters, pagination, sort } = state;

        // Construct query parameters
        const params = new URLSearchParams({
            page: pagination.page,
            pageSize: pagination.pageSize,
            sortBy: sort.field,
            sortDirection: sort.direction
        });

        // Add filters
        if (filters.search) params.append('search', filters.search);
        if (filters.categoryId) params.append('categoryId', filters.categoryId);
        if (filters.subcategoryId) params.append('subcategoryId', filters.subcategoryId);
        if (filters.hasPrerequisites) params.append('hasPrerequisites', true);
        if (filters.isRacial) params.append('isRacial', true);
        if (filters.types.size) {
            Array.from(filters.types).forEach(type => {
                params.append('types[]', type);
            });
        }

        // Add stat ranges
        Object.entries(filters.statRanges).forEach(([stat, range]) => {
            if (range.min) params.append(`${stat}Min`, range.min);
            if (range.max) params.append(`${stat}Max`, range.max);
        });

        this.state.setLoading(true);

        const response = await fetch(`/api/classes?${params}`, {
            signal: this.controller.signal
        });

        const data = await response.json();

        this.state.setTotal(data.total);
        return data.classes;

    } catch (error) {
        if (error.name === 'AbortError') return null;
        throw error;
    } finally {
        this.state.setLoading(false);
        this.controller = null;
    }
}

async fetchClassDetails(classId) {
    try {
        const response = await fetch(`/api/classes/${classId}/details`);
        return await response.json();
    } catch (error) {
        console.error('Error fetching class details:', error);
        throw error;
    }
}
}

export const classService = new ClassService(classBrowserState);