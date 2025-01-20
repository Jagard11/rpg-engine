// ./FrontEnd/CharacterInfo/scripts/components/filterPanel.js

class FilterPanel {
constructor(container, onFilterChange) {
    this.container = container;
    this.onFilterChange = onFilterChange;
    this.activeFilters = new Set();
    this.init();
}

init() {
    this.render();
    this.bindEvents();
}

render() {
    this.container.innerHTML = `
        <div class="filter-panel space-y-6">
            <!-- Type Filters -->
            <div class="filter-section">
                <h3 class="text-sm font-medium mb-2">Class Types</h3>
                <div class="space-y-2">
                    <label class="flex items-center gap-2">
                        <input type="checkbox" name="type" value="base" class="form-checkbox">
                        <span class="text-sm">Base Classes</span>
                    </label>
                    <label class="flex items-center gap-2">
                        <input type="checkbox" name="type" value="high" class="form-checkbox">
                        <span class="text-sm">High Classes</span>
                    </label>
                    <label class="flex items-center gap-2">
                        <input type="checkbox" name="type" value="rare" class="form-checkbox">
                        <span class="text-sm">Rare Classes</span>
                    </label>
                </div>
            </div>

            <!-- Additional Filters -->
            <div class="filter-section">
                <h3 class="text-sm font-medium mb-2">Requirements</h3>
                <div class="space-y-2">
                    <label class="flex items-center gap-2">
                        <input type="checkbox" name="has_prerequisites" class="form-checkbox">
                        <span class="text-sm">Has Prerequisites</span>
                    </label>
                    <label class="flex items-center gap-2">
                        <input type="checkbox" name="racial_class" class="form-checkbox">
                        <span class="text-sm">Racial Classes</span>
                    </label>
                </div>
            </div>

            <!-- Stats Range Filters -->
            <div class="filter-section">
                <h3 class="text-sm font-medium mb-2">Stat Ranges</h3>
                <div class="space-y-4">
                    <div>
                        <label class="text-sm block mb-1">Base HP</label>
                        <div class="flex gap-2">
                            <input type="number" name="hp_min" placeholder="Min" 
                                class="w-20 px-2 py-1 text-sm border rounded">
                            <input type="number" name="hp_max" placeholder="Max"
                                class="w-20 px-2 py-1 text-sm border rounded">
                        </div>
                    </div>
                    <div>
                        <label class="text-sm block mb-1">Base MP</label>
                        <div class="flex gap-2">
                            <input type="number" name="mp_min" placeholder="Min"
                                class="w-20 px-2 py-1 text-sm border rounded">
                            <input type="number" name="mp_max" placeholder="Max"
                                class="w-20 px-2 py-1 text-sm border rounded">
                        </div>
                    </div>
                </div>
            </div>

            <!-- Active Filters Display -->
            <div class="filter-section" id="active-filters">
                <h3 class="text-sm font-medium mb-2">Active Filters</h3>
                <div class="flex flex-wrap gap-2" id="filter-tags"></div>
            </div>

            <!-- Reset Button -->
            <button class="w-full py-2 px-4 bg-gray-100 hover:bg-gray-200 
                        rounded transition-colors text-sm" id="reset-filters">
                Reset All Filters
            </button>
        </div>
    `;
}

bindEvents() {
    // Type filters
    this.container.querySelectorAll('input[name="type"]').forEach(input => {
        input.addEventListener('change', () => this.handleTypeFilter(input));
    });

    // Additional filters
    this.container.querySelectorAll('input[type="checkbox"]').forEach(input => {
        if (input.name !== "type") {
            input.addEventListener('change', () => this.handleToggleFilter(input));
        }
    });

    // Stat range filters
    this.container.querySelectorAll('input[type="number"]').forEach(input => {
        input.addEventListener('change', () => this.handleStatRangeFilter(input));
    });

    // Reset button
    this.container.querySelector('#reset-filters').addEventListener('click', () => {
        this.resetFilters();
    });
}

handleTypeFilter(input) {
    const filterId = `type_${input.value}`;
    if (input.checked) {
        this.activeFilters.add({
            id: filterId,
            type: 'type',
            value: input.value,
            label: `Type: ${input.value.charAt(0).toUpperCase() + input.value.slice(1)}`
        });
    } else {
        this.activeFilters.delete(filterId);
    }
    this.updateFilterTags();
    this.emitChange();
}

handleToggleFilter(input) {
    const filterId = input.name;
    if (input.checked) {
        this.activeFilters.add({
            id: filterId,
            type: 'toggle',
            value: true,
            label: this.getFilterLabel(input.name)
        });
    } else {
        this.activeFilters.delete(filterId);
    }
    this.updateFilterTags();
    this.emitChange();
}

handleStatRangeFilter(input) {
    const [stat, bound] = input.name.split('_');
    const filterId = `${stat}_${bound}`;
    const value = input.value;

    if (value) {
        this.activeFilters.add({
            id: filterId,
            type: 'range',
            stat,
            bound,
            value,
            label: `${stat.toUpperCase()} ${bound === 'min' ? '>=' : '<='} ${value}`
        });
    } else {
        this.activeFilters.delete(filterId);
    }
    this.updateFilterTags();
    this.emitChange();
}

getFilterLabel(name) {
    const labels = {
        has_prerequisites: 'Has Prerequisites',
        racial_class: 'Racial Classes Only'
    };
    return labels[name] || name;
}

updateFilterTags() {
    const container = this.container.querySelector('#filter-tags');
    container.innerHTML = '';

    Array.from(this.activeFilters).forEach(filter => {
        const tag = document.createElement('div');
        tag.className = 'inline-flex items-center gap-1 px-2 py-1 bg-blue-100 text-blue-800 rounded-full text-sm';
        tag.innerHTML = `
            <span>${filter.label}</span>
            <button class="hover:text-blue-600" data-filter-id="${filter.id}">
                <i data-lucide="x" class="h-3 w-3"></i>
            </button>
        `;
        
        tag.querySelector('button').addEventListener('click', () => {
            this.removeFilter(filter.id);
        });
        
        container.appendChild(tag);
    });

    // Update Lucide icons
    if (window.lucide) {
        window.lucide.createIcons();
    }

    // Show/hide section based on active filters
    const section = this.container.querySelector('#active-filters');
    section.style.display = this.activeFilters.size ? 'block' : 'none';
}

removeFilter(filterId) {
    const filter = Array.from(this.activeFilters).find(f => f.id === filterId);
    if (filter) {
        // Uncheck corresponding input
        if (filter.type === 'type' || filter.type === 'toggle') {
            const input = this.container.querySelector(`input[name="${filter.type === 'type' ? 'type' : filterId}"]${filter.type === 'type' ? `[value="${filter.value}"]` : ''}`);
            if (input) {
                input.checked = false;
            }
        } else if (filter.type === 'range') {
            const input = this.container.querySelector(`input[name="${filter.stat}_${filter.bound}"]`);
            if (input) {
                input.value = '';
            }
        }

        this.activeFilters.delete(filter);
        this.updateFilterTags();
        this.emitChange();
    }
}

resetFilters() {
    // Clear all checkboxes
    this.container.querySelectorAll('input[type="checkbox"]').forEach(input => {
        input.checked = false;
    });

    // Clear all number inputs
    this.container.querySelectorAll('input[type="number"]').forEach(input => {
        input.value = '';
    });

    this.activeFilters.clear();
    this.updateFilterTags();
    this.emitChange();
}

emitChange() {
    if (this.onFilterChange) {
        this.onFilterChange(Array.from(this.activeFilters));
    }
}
}