// ./FrontEnd/CharacterInfo/scripts/components/classCard.js

class ClassCard {
    constructor(classData, onExpand) {
        this.data = classData;
        this.onExpand = onExpand;
        this.expanded = false;
        this.detailsLoaded = false;
        this.element = this.create();
    }

    create() {
        const card = document.createElement('div');
        card.className = 'class-card border rounded-lg p-4 bg-white shadow-sm';
        card.dataset.classId = this.data.id;

        // Create header section
        const header = this.createHeader();
        card.appendChild(header);

        // Create details section
        const details = this.createDetails();
        card.appendChild(details);

        return card;
    }

    createHeader() {
        const header = document.createElement('div');
        header.className = 'flex justify-between items-start';

        // Create info section
        const info = document.createElement('div');
        info.className = 'flex items-start gap-3';

        // Add appropriate icon based on class type
        const iconWrapper = document.createElement('div');
        iconWrapper.className = 'mt-1';
        const icon = document.createElement('i');
        icon.setAttribute('data-lucide', this.getClassIcon());
        icon.className = this.getIconClass();
        iconWrapper.appendChild(icon);

        // Add title and subtitle
        const titleSection = document.createElement('div');
        const title = document.createElement('h3');
        title.className = 'font-semibold text-lg';
        title.textContent = this.data.name;

        const subtitle = document.createElement('div');
        subtitle.className = 'flex gap-2 text-sm text-gray-500';
        subtitle.innerHTML = `
            <span>${this.data.type}</span>
            <span>•</span>
            <span>${this.data.category}</span>
            ${this.data.subcategory ? `
                <span>•</span>
                <span>${this.data.subcategory}</span>
            ` : ''}
        `;

        titleSection.appendChild(title);
        titleSection.appendChild(subtitle);

        info.appendChild(iconWrapper);
        info.appendChild(titleSection);

        // Create expand button
        const expandBtn = document.createElement('button');
        expandBtn.className = 'expand-btn text-gray-400 hover:text-gray-600 transition-colors';
        expandBtn.innerHTML = '<i data-lucide="chevron-down"></i>';
        expandBtn.addEventListener('click', () => this.toggle());

        header.appendChild(info);
        header.appendChild(expandBtn);

        return header;
    }

    createDetails() {
        const details = document.createElement('div');
        details.className = 'class-details hidden mt-4 pt-4 border-t space-y-4';

        // Add description
        const description = document.createElement('div');
        description.innerHTML = `
            <h4 class="text-sm font-medium mb-1">Description</h4>
            <p class="text-sm text-gray-600">${this.data.description || 'No description available.'}</p>
        `;
        details.appendChild(description);

        // Add base stats
        const stats = document.createElement('div');
        stats.innerHTML = `
            <h4 class="text-sm font-medium mb-1">Base Stats</h4>
            <div class="grid grid-cols-3 gap-2 text-sm">
                <div>HP: ${this.data.base_hp}</div>
                <div>MP: ${this.data.base_mp}</div>
                <div>ATK: ${this.data.base_physical_attack}</div>
                <div>DEF: ${this.data.base_physical_defense}</div>
                <div>AGI: ${this.data.base_agility}</div>
                <div>MAG: ${this.data.base_magical_attack}</div>
                <div>MDEF: ${this.data.base_magical_defense}</div>
                <div>RES: ${this.data.base_resistance}</div>
                <div>SP: ${this.data.base_special}</div>
            </div>
        `;
        details.appendChild(stats);

        // Add placeholder for prerequisites and exclusions
        const additional = document.createElement('div');
        additional.className = 'additional-details';
        additional.innerHTML = '<div class="text-sm text-gray-500">Loading additional details...</div>';
        details.appendChild(additional);

        return details;
    }

    async toggle() {
        const details = this.element.querySelector('.class-details');
        const icon = this.element.querySelector('.expand-btn i');
        const isHidden = details.classList.contains('hidden');

        // Update UI
        details.classList.toggle('hidden');
        icon.setAttribute('data-lucide', isHidden ? 'chevron-up' : 'chevron-down');

        // Load details if needed
        if (isHidden && !this.detailsLoaded) {
            await this.loadDetails();
        }

        // Update icons
        if (window.lucide) {
            window.lucide.createIcons();
        }

        this.expanded = !isHidden;
        if (this.onExpand) {
            this.onExpand(this.expanded);
        }
    }

    async loadDetails() {
        const additional = this.element.querySelector('.additional-details');
        
        try {
            const response = await fetch(`/api/classes/${this.data.id}/details`);
            const details = await response.json();
            
            additional.innerHTML = this.renderAdditionalDetails(details);
            this.detailsLoaded = true;
        } catch (error) {
            console.error('Error loading class details:', error);
            additional.innerHTML = '<div class="text-red-500">Error loading details</div>';
        }
    }

    renderAdditionalDetails(details) {
        return `
            <div class="space-y-4">
                ${this.renderPrerequisites(details.prerequisites)}
                ${this.renderExclusions(details.exclusions)}
            </div>
        `;
    }

    renderPrerequisites(prerequisites) {
        if (!prerequisites.length) {
            return '';
        }

        const formatPrerequisite = (prereq) => {
            switch (prereq.prerequisite_type) {
                case 'specific_class':
                    return `${prereq.target_name} (Level ${prereq.required_level})`;
                case 'category_total':
                    return `${prereq.category_name} Total Level ${prereq.required_level}`;
                case 'karma':
                    return `Karma between ${prereq.min_value} and ${prereq.max_value}`;
                default:
                    return 'Unknown prerequisite type';
            }
        };

        // Group prerequisites by prerequisite_group
        const groups = prerequisites.reduce((acc, prereq) => {
            const group = prereq.prerequisite_group;
            if (!acc[group]) {
                acc[group] = [];
            }
            acc[group].push(prereq);
            return acc;
        }, {});

        return `
            <div>
                <h4 class="text-sm font-medium mb-1">Prerequisites</h4>
                <div class="space-y-2">
                    ${Object.entries(groups).map(([groupId, groupPrereqs]) => `
                        <div class="text-sm text-gray-600">
                            ${groupPrereqs.map(prereq => formatPrerequisite(prereq)).join(' OR ')}
                        </div>
                    `).join('')}
                </div>
            </div>
        `;
    }

    renderExclusions(exclusions) {
        if (!exclusions.length) {
            return '';
        }

        const formatExclusion = (excl) => {
            switch (excl.exclusion_type) {
                case 'specific_class':
                    return excl.target_name;
                case 'category_total':
                    return `${excl.category_name} (Total Level ${excl.min_value}+)`;
                case 'karma':
                    return `Karma between ${excl.min_value} and ${excl.max_value}`;
                case 'racial_total':
                    return `Racial levels ${excl.min_value}+`;
                default:
                    return 'Unknown exclusion type';
            }
        };

        return `
            <div>
                <h4 class="text-sm font-medium mb-1">Exclusions</h4>
                <ul class="text-sm text-gray-600">
                    ${exclusions.map(excl => `
                        <li>• ${formatExclusion(excl)}</li>
                    `).join('')}
                </ul>
            </div>
        `;
    }

    getClassIcon() {
        if (this.data.is_racial) {
            return 'user';
        }
        switch (this.data.type.toLowerCase()) {
            case 'base':
                return 'shield';
            case 'high':
                return 'swords';
            case 'rare':
                return 'crown';
            default:
                return 'circle';
        }
    }

    getIconClass() {
        let colorClass = 'text-gray-400';
        if (this.data.is_racial) {
            colorClass = 'text-purple-500';
        } else {
            switch (this.data.type.toLowerCase()) {
                case 'base':
                    colorClass = 'text-blue-500';
                    break;
                case 'high':
                    colorClass = 'text-green-500';
                    break;
                case 'rare':
                    colorClass = 'text-yellow-500';
                    break;
            }
        }
        return `h-5 w-5 ${colorClass}`;
    }
    }

