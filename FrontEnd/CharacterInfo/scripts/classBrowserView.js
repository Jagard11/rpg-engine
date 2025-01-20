// ./FrontEnd/CharacterInfo/scripts/classBrowserView.js

class ClassBrowserView {
    constructor(container) {
        this.container = container;
        this.classes = [];
        this.state = classBrowserState;
        this.service = classService;
        
        this.categoryTree = null;
        this.filterPanel = null;
        this.cards = new Map();
        
        this.init();
    }

    async init() {
        // Create layout
        this.createLayout();
        
        // Initialize components
        this.initializeComponents();
        
        // Subscribe to state changes
        this.state.subscribe(() => this.onStateChange());
        
        // Load initial data
        await this.loadClasses();
        
        // Bind scroll event for infinite loading
        this.bindInfiniteScroll();
    }

    createLayout() {
        this.container.innerHTML = `
            <div class="class-browser flex h-screen">
                <!-- Left Sidebar -->
                <div class="w-64 bg-gray-50 border-r overflow-y-auto">
                    <div id="filter-panel"></div>
                </div>
                
                <!-- Main Content -->
                <div class="flex-1 flex flex-col">
                    <!-- Search Bar -->
                    <div class="p-4 border-b bg-white">
                        <div class="relative">
                            <input 
                                type="text" 
                                id="class-search"
                                placeholder="Search classes..."
                                class="w-full pl-10 pr-4 py-2 border rounded-md"
                            >
                            <i data-lucide="search" 
                               class="absolute left-3 top-2.5 h-5 w-5 text-gray-400">
                            </i>
                        </div>
                    </div>
                    
                    <!-- Sort Controls -->
                    <div class="px-4 py-2 border-b bg-white flex justify-between items-center">
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
                        <div id="results-count" class="text-sm text-gray-600"></div>
                    </div>
                    
                    <!-- Class Cards -->
                    <div class="flex-1 overflow-y-auto p-4 bg-gray-100">
                        <div id="class-cards" class="grid grid-cols-1 md:grid-cols-2 gap-4"></div>
                        
                        <!-- Loading Indicator -->
                        <div id="loading-indicator" class="hidden py-4 text-center">
                            <div class="inline-flex items-center gap-2">
                                <i data-lucide="loader" class="animate-spin"></i>
                                Loading more classes...
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        `;

        // Initialize Lucide icons
        if (window.lucide) {
            window.lucide.createIcons();
        }
    }

    initializeComponents() {
        // Initialize Filter Panel
        this.filterPanel = new FilterPanel(
            document.getElementById('filter-panel'),
            filters => this.handleFilterChange(filters)
        );

        // Initialize Category Tree (part of Filter Panel)
        const categoryContainer = document.getElementById('category-tree');
        if (categoryContainer) {
            this.categoryTree = new CategoryTree(
                categoryContainer,
                (categoryId, subcategoryId) => this.handleCategorySelect(categoryId, subcategoryId)
            );
        }

        // Initialize search input
        const searchInput = document.getElementById('class-search');
        searchInput.addEventListener('input', _.debounce(e => {
            this.state.setSearch(e.target.value);
        }, 300));

        // Initialize sort controls
        const sortSelect = document.getElementById('sort-select');
        sortSelect.addEventListener('change', e => {
            this.state.setSort(e.target.value, this.state.getState().sort.direction);
        });

        const sortDirection = document.getElementById('sort-direction');
        sortDirection.addEventListener('click', () => {
            const currentDirection = this.state.getState().sort.direction;
            const newDirection = currentDirection === 'asc' ? 'desc' : 'asc';
            this.state.setSort(this.state.getState().sort.field, newDirection);
            
            // Update icon
            const icon = sortDirection.querySelector('i');
            icon.setAttribute('data-lucide', newDirection === 'asc' ? 'arrow-up' : 'arrow-down');
            if (window.lucide) {
                window.lucide.createIcons();
            }
        });
    }

    async loadClasses() {
        try {
            // Show loading indicator
            const loadingIndicator = document.getElementById('loading-indicator');
            loadingIndicator.classList.remove('hidden');

            // Fetch classes
            const classes = await this.service.fetchClasses();
            if (!classes) return; // Request was cancelled

            // Update state
            this.classes = classes;
            this.renderClasses();
            this.updateResultsCount();

        } catch (error) {
            console.error('Error loading classes:', error);
            // Show error message
            const cardsContainer = document.getElementById('class-cards');
            cardsContainer.innerHTML = `
                <div class="col-span-full text-center py-8 text-red-500">
                    Error loading classes. Please try again.
                </div>
            `;
        } finally {
            // Hide loading indicator
            const loadingIndicator = document.getElementById('loading-indicator');
            loadingIndicator.classList.add('hidden');
        }
    }

    renderClasses() {
        const container = document.getElementById('class-cards');
        
        // Clear existing cards if on first page
        if (this.state.getState().pagination.page === 1) {
            container.innerHTML = '';
            this.cards.clear();
        }

        // Create and append new cards
        this.classes.forEach(classData => {
            if (!this.cards.has(classData.id)) {
                const card = new ClassCard(
                    classData,
                    expanded => this.handleCardExpand(classData.id, expanded)
                );
                container.appendChild(card.element);
                this.cards.set(classData.id, card);
            }
        });

        // Update Lucide icons
        if (window.lucide) {
            window.lucide.createIcons();
        }
    }

    updateResultsCount() {
        const counter = document.getElementById('results-count');
        const state = this.state.getState();
        const { total } = state.pagination;
        const displayed = this.cards.size;
        
        counter.textContent = `Showing ${displayed} of ${total} classes`;
    }

    handleFilterChange(filters) {
        // Process each filter
        filters.forEach(filter => {
            switch (filter.type) {
                case 'type':
                    this.state.setFilter('type', {
                        active: true,
                        value: filter.value
                    });
                    break;
                case 'toggle':
                    switch (filter.id) {
                        case 'has_prerequisites':
                            this.state.setFilter('prerequisites', true);
                            break;
                        case 'racial_class':
                            this.state.setFilter('racial', true);
                            break;
                    }
                    break;
                case 'range':
                    this.state.setFilter('stat_range', {
                        stat: filter.stat,
                        bound: filter.bound,
                        value: filter.value
                    });
                    break;
            }
        });
    }

    handleCategorySelect(categoryId, subcategoryId) {
        this.state.setFilter('category', categoryId);
        if (subcategoryId) {
            this.state.setFilter('subcategory', subcategoryId);
        }
    }

    handleCardExpand(classId, expanded) {
        if (expanded) {
            // Load details if not already loaded
            const card = this.cards.get(classId);
            if (card && !card.detailsLoaded) {
                this.loadClassDetails(classId);
            }
        }
    }

    async loadClassDetails(classId) {
        const card = this.cards.get(classId);
        if (!card) return;

        try {
            const details = await this.service.fetchClassDetails(classId);
            card.updateDetails(details);
        } catch (error) {
            console.error('Error loading class details:', error);
            card.showError();
        }
    }

    bindInfiniteScroll() {
        const container = document.querySelector('.class-browser .flex-1.overflow-y-auto');
        container.addEventListener('scroll', _.debounce(() => {
            const { scrollTop, scrollHeight, clientHeight } = container;
            
            // Check if we're near the bottom
            if (scrollTop + clientHeight >= scrollHeight - 100) {
                this.loadMoreClasses();
            }
        }, 100));
    }

    async loadMoreClasses() {
        const state = this.state.getState();
        const { loading, page, pageSize, total } = state.pagination;

        // Check if we should load more
        if (!loading && this.cards.size < total) {
            this.state.nextPage();
        }
    }

    async onStateChange() {
        const state = this.state.getState();
        const { pagination } = state;

        // Update sort indicator
        const sortButton = document.getElementById('sort-direction');
        const sortIcon = sortButton.querySelector('i');
        sortIcon.setAttribute('data-lucide', state.sort.direction === 'asc' ? 'arrow-up' : 'arrow-down');
        if (window.lucide) {
            window.lucide.createIcons();
        }

        // Load classes with new state
        if (pagination.page === 1) {
            // Full reload
            await this.loadClasses();
        } else {
            // Load more
            await this.loadMoreClasses();
        }
    }
}

// Export the view
export default ClassBrowserView;

// Initialize the class browser
document.addEventListener('DOMContentLoaded', () => {
    const container = document.getElementById('class-browser-container');
    if (container) {
        new ClassBrowserView(container);
    }
});