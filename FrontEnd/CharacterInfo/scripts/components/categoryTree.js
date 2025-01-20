// ./FrontEnd/CharacterInfo/scripts/components/categoryTree.js

class CategoryTree {
    constructor(container, onSelect) {
        this.container = container;
        this.onSelect = onSelect;
        this.categories = [];
        this.selectedId = null;
    }

    async init() {
        try {
            const response = await fetch('/api/class-categories');
            this.categories = await response.json();
            this.render();
        } catch (error) {
            console.error('Error loading categories:', error);
            this.container.innerHTML = `
                <div class="text-red-500">Error loading categories</div>
            `;
        }
    }

    render() {
        const createCategoryNode = (category) => {
            const div = document.createElement('div');
            div.className = 'category-node';
            
            const header = document.createElement('div');
            header.className = `
                category-header flex items-center gap-2 py-1 px-2 rounded
                hover:bg-gray-200 cursor-pointer
                ${this.selectedId === category.id ? 'bg-gray-200' : ''}
            `;
            
            const icon = document.createElement('i');
            icon.setAttribute('data-lucide', category.subcategories.length ? 'chevron-right' : 'minus');
            icon.className = 'h-4 w-4';
            
            const name = document.createElement('span');
            name.textContent = category.name;
            name.className = 'text-sm';
            
            header.appendChild(icon);
            header.appendChild(name);
            div.appendChild(header);

            if (category.subcategories.length) {
                const subContainer = document.createElement('div');
                subContainer.className = 'subcategories ml-4 hidden';
                
                category.subcategories.forEach(sub => {
                    const subDiv = document.createElement('div');
                    subDiv.className = `
                        subcategory py-1 px-2 rounded text-sm
                        hover:bg-gray-200 cursor-pointer
                    `;
                    subDiv.textContent = sub.name;
                    
                    subDiv.addEventListener('click', (e) => {
                        e.stopPropagation();
                        this.handleSelect(category.id, sub.id);
                    });
                    
                    subContainer.appendChild(subDiv);
                });
                
                div.appendChild(subContainer);
                
                header.addEventListener('click', () => {
                    const isExpanded = subContainer.classList.contains('hidden');
                    icon.setAttribute('data-lucide', isExpanded ? 'chevron-down' : 'chevron-right');
                    subContainer.classList.toggle('hidden');
                });
            } else {
                header.addEventListener('click', () => {
                    this.handleSelect(category.id);
                });
            }

            return div;
        };

        // Group categories by racial/non-racial
        const racialCategories = this.categories.filter(c => c.is_racial);
        const nonRacialCategories = this.categories.filter(c => !c.is_racial);

        this.container.innerHTML = '';

        // Render racial categories
        if (racialCategories.length) {
            const racialSection = document.createElement('div');
            racialSection.className = 'mb-4';
            
            const racialHeader = document.createElement('h4');
            racialHeader.className = 'text-sm font-medium mb-2 text-gray-500';
            racialHeader.textContent = 'Racial Categories';
            
            racialSection.appendChild(racialHeader);
            racialCategories.forEach(category => {
                racialSection.appendChild(createCategoryNode(category));
            });
            
            this.container.appendChild(racialSection);
        }

        // Render non-racial categories
        if (nonRacialCategories.length) {
            const nonRacialSection = document.createElement('div');
            
            const nonRacialHeader = document.createElement('h4');
            nonRacialHeader.className = 'text-sm font-medium mb-2 text-gray-500';
            nonRacialHeader.textContent = 'Job Categories';
            
            nonRacialSection.appendChild(nonRacialHeader);
            nonRacialCategories.forEach(category => {
                nonRacialSection.appendChild(createCategoryNode(category));
            });
            
            this.container.appendChild(nonRacialSection);
        }

        // Initialize Lucide icons
        if (window.lucide) {
            window.lucide.createIcons();
        }
    }

    handleSelect(categoryId, subcategoryId = null) {
        this.selectedId = categoryId;
        this.render();
        if (this.onSelect) {
            this.onSelect(categoryId, subcategoryId);
        }
    }
}

