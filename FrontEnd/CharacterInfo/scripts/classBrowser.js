// ./FrontEnd/CharacterInfo/scripts/classBrowser.js

class ClassBrowser {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.state = {
            classes: [],
            filters: {
                search: '',
                categoryId: null,
                classType: null,
                isRacial: false,
                hasPrerequisites: false
            },
            pagination: {
                page: 1,
                pageSize: 20,
                total: 0
            },
            expandedCardId: null
        };
        
        this.init();
    }

    async init() {
        this.createLayout();
        await this.loadInitialData();
        this.bindEvents();
    }

    createLayout() {
        this.container.innerHTML = `
            <div class="class-browser flex h-screen">
                <!-- Filter Sidebar -->
                <div class="filter-sidebar w-64 bg-gray-50 p-4 border-r overflow-y-auto">
                    <div class="flex items-center gap-2 mb-6">
                        <i data-lucide="filter" class="h-5 w-5"></i>
                        <h2 class="text-lg font-semibold">Filters</h2>
                    </div>
                    
                    <!-- Search Bar -->
                    <div class="mb-4">
                        <div class="relative">
                            <input 
                                type="text" 
                                id="class-search"
                                class="w-full pl-10 pr-4 py-2 border rounded-md"
                                placeholder="Search classes..."
                            >
                            <i data-lucide="search" 
                               class="absolute left-3 top-2.5 h-5 w-5 text-gray-400">
                            </i>
                        </div>
                    </div>

                    <!-- Category Tree -->
                    <div class="mb-4">
                        <h3 class="font-medium mb-2">Categories</h3>
                        <div id="category-tree" class="pl-2"></div>
                    </div>

                    <!-- Class Type Filter -->
                    <div class="mb-4">
                        <h3 class="font-medium mb-2">Class Types</h3>
                        <div class="space-y-2" id="class-type-filters"></div>
                    </div>

                    <!-- Additional Filters -->
                    <div class="mb-4">
                        <h3 class="font-medium mb-2">Additional Filters</h3>
                        <div class="space-y-2">
                            <label class="flex items-center gap-2">
                                <input type="checkbox" id="racial-filter">
                                <span>Show Racial Classes</span>
                            </label>
                            <label class="flex items-center gap-2">
                                <input type="checkbox" id="prereq-filter">
                                <span>Has Prerequisites</span>
                            </label>
                        </div>
                    </div>
                </div>

                <!-- Main Content -->
                <div class="flex-1 p-4 overflow-y-auto">
                    <!-- Active Filters -->
                    <div id="active-filters" class="mb-4 flex flex-wrap gap-2"></div>

                    <!-- Sort Controls -->
                    <div class="mb-4 flex justify-between items-center">
                        <div class="flex items-center gap-2">
                            <select id="sort-select" class="border rounded-md p-2">
                                <option value="name">Name</option>
                                <option value="type">Type</option>
                                <option value="category">Category</option>
                            </select>
                            <button id="sort-direction" class="p-2 border rounded-md">
                                <i data-lucide="arrow-up-down"></i>
                            </button>
                        </div>
                        <div class="text-sm text-gray-600" id="results-count"></div>
                    </div>

                    <!-- Class Cards Container -->
                    <div id="class-cards" class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
                    </div>

                    <!-- Loading Indicator -->
                    <div id="loading-indicator" class="hidden text-center py-4">
                        Loading more classes...
                    </div>
                </div>
            </div>
        `;
    }

    async loadInitialData() {
        try {
            // Load categories for the tree
            const categories = await this.fetchCategories();
            this.renderCategoryTree(categories);

            // Load class types
            const classTypes = await this.fetchClassTypes();
            this.renderClassTypeFilters(classTypes);

            // Load initial set of classes
            await this.fetchAndRenderClasses();
        } catch (error) {
            console.error('Error loading initial data:', error);
        }
    }

    async fetchCategories() {
        // Fetch categories from database
        const response = await fetch('/api/class-categories');
        return await response.json();
    }

    async fetchClassTypes() {
        // Fetch class types from database
        const response = await fetch('/api/class-types');
        return await response.json();
    }

    async fetchAndRenderClasses() {
        const { page, pageSize } = this.state.pagination;
        const filters = this.state.filters;

        try {
            // Construct query parameters
            const params = new URLSearchParams({
                page,
                pageSize,
                ...filters
            });

            const response = await fetch(`/api/classes?${params}`);
            const data = await response.json();

            this.state.pagination.total = data.total;
            this.renderClasses(data.classes);
            this.updateResultsCount();
        } catch (error) {
            console.error('Error fetching classes:', error);
        }
    }

    renderClasses(classes) {
        const container = document.getElementById('class-cards');
        
        classes.forEach(classData => {
            const card = this.createClassCard(classData);
            container.appendChild(card);
        });
    }

    createClassCard(classData) {
        const card = document.createElement('div');
        card.className = 'class-card border rounded-lg p-4 bg-white shadow-sm';
        
        card.innerHTML = `
            <div class="flex justify-between items-start">
                <div>
                    <h3 class="font-semibold">${classData.name}</h3>
                    <p class="text-sm text-gray-500">
                        ${classData.type} • ${classData.category}
                    </p>
                </div>
                <button class="expand-btn text-gray-400 hover:text-gray-600">
                    <i data-lucide="chevron-down"></i>
                </button>
            </div>
            <div class="class-details hidden mt-4 pt-4 border-t">
                <p class="text-sm text-gray-600">${classData.description}</p>
                <!-- Additional details will be loaded when expanded -->
            </div>
        `;

        return card;
    }

    renderCategoryTree(categories) {
        const container = document.getElementById('category-tree');
        // Implementation for rendering hierarchical category tree
    }

    renderClassTypeFilters(types) {
        const container = document.getElementById('class-type-filters');
        // Implementation for rendering class type filter buttons
    }

    updateResultsCount() {
        const counter = document.getElementById('results-count');
        counter.textContent = `Showing ${this.state.classes.length} of ${this.state.pagination.total} classes`;
    }

    bindEvents() {
        // Search input handling
        const searchInput = document.getElementById('class-search');
        searchInput.addEventListener('input', _.debounce(() => {
            this.state.filters.search = searchInput.value;
            this.state.pagination.page = 1;
            this.fetchAndRenderClasses();
        }, 300));

        // Infinite scroll handling
        const content = document.querySelector('.class-browser > div:last-child');
        content.addEventListener('scroll', _.debounce(() => {
            const { scrollTop, scrollHeight, clientHeight } = content;
            
            if (scrollTop + clientHeight >= scrollHeight - 100) {
                this.loadMoreClasses();
            }
        }, 100));

        // Card expansion handling
        document.getElementById('class-cards').addEventListener('click', (e) => {
            const expandBtn = e.target.closest('.expand-btn');
            if (expandBtn) {
                const card = expandBtn.closest('.class-card');
                const details = card.querySelector('.class-details');
                const icon = expandBtn.querySelector('i');
                
                details.classList.toggle('hidden');
                icon.dataset.lucide = details.classList.contains('hidden') ? 
                    'chevron-down' : 'chevron-up';
                
                if (!details.classList.contains('hidden') && !details.dataset.loaded) {
                    this.loadClassDetails(card.dataset.classId, details);
                }
            }
        });
    }

    async loadMoreClasses() {
        if (this.state.classes.length >= this.state.pagination.total) return;
        
        this.state.pagination.page += 1;
        document.getElementById('loading-indicator').classList.remove('hidden');
        
        await this.fetchAndRenderClasses();
        
        document.getElementById('loading-indicator').classList.add('hidden');
    }

    async loadClassDetails(classId, container) {
        try {
            const response = await fetch(`/api/classes/${classId}/details`);
            const details = await response.json();
            
            container.innerHTML = this.renderClassDetails(details);
            container.dataset.loaded = 'true';
        } catch (error) {
            console.error('Error loading class details:', error);
            container.innerHTML = '<p class="text-red-500">Error loading details</p>';
        }
    }

    renderClassDetails(details) {
        return `
            <div class="space-y-4">
                <div>
                    <h4 class="text-sm font-medium mb-1">Base Stats</h4>
                    <div class="grid grid-cols-3 gap-2">
                        <div class="text-sm">HP: ${details.baseHp}</div>
                        <div class="text-sm">MP: ${details.baseMp}</div>
                        <div class="text-sm">ATK: ${details.baseAtk}</div>
                    </div>
                </div>
                
                <div>
                    <h4 class="text-sm font-medium mb-1">Prerequisites</h4>
                    <ul class="text-sm text-gray-600">
                        ${details.prerequisites.map(p => `<li>${p}</li>`).join('')}
                    </ul>
                </div>
                
                <div>
                    <h4 class="text-sm font-medium mb-1">Exclusions</h4>
                    <ul class="text-sm text-gray-600">
                        ${details.exclusions.map(e => `<li>${e}</li>`).join('')}
                    </ul>
                </div>
            </div>
        `;
    }
}

// Export for use in other files
export default ClassBrowser;