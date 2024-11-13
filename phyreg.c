#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <yaml.h>

#include "phytool.h"

/* Our example parser states. */
enum state_value {
    START,
    ACCEPT_LIST_CLASS,
    ACCEPT_LIST_REG,
    ACCEPT_VALUES,
    ACCEPT_VALUES1,
    ACCEPT_REG_NAME,
    ACCEPT_KEY,
    ACCEPT_KEY1,
    ACCEPT_REG_FIELD_KEY,
    ACCEPT_VALUE,
    ACCEPT_VALUE1,
    ACCEPT_REG_FIELD_VALUE,
    STOP,
    ERROR,
};

struct parser_state {
    enum state_value state;
    int accepted;
    int error;
    char *key;
    char *value;

    struct phy_desc *phy;
};

int parse_number(const char *str) {
    if (strncasecmp(str, "0x", 2) == 0) {
        return (int)strtol(str + 2, NULL, 16);
    }
    return (int)strtol(str, NULL, 10);
}

struct num_list parse_range(const char *range) {
    struct num_list list = {NULL, 0};
    char *start_str, *end_str;
    int start, end;

    start_str = strdup(range);
    end_str = strchr(start_str, '-');
    if (end_str == NULL) {
        free(start_str);
        return list;
    }
    *end_str = '\0';
    end_str++;

    start = parse_number(start_str);
    end = parse_number(end_str);

    list.count = end - start + 1;
    list.numbers = malloc(list.count * sizeof(int));
    for (int i = 0; i < list.count; i++) {
        list.numbers[i] = start + i;
    }

    free(start_str);
    return list;
}

static struct num_list parse_input(const char *input) {
    struct num_list result = {NULL, 0};
    char *token, *input_copy, *next_token;

    input_copy = strdup(input);
    token = strtok_r(input_copy, ", ", &next_token);
    while (token != NULL) {
        if (strchr(token, '-') != NULL) {
            struct num_list range_list = parse_range(token);
            result.numbers = realloc(result.numbers, (result.count + range_list.count) * sizeof(int));
            for (int i = 0; i < range_list.count; i++) {
                result.numbers[result.count + i] = range_list.numbers[i];
            }
            result.count += range_list.count;
            free(range_list.numbers);
        } else {
            int number = parse_number(token);
            result.numbers = realloc(result.numbers, (result.count + 1) * sizeof(int));
            result.numbers[result.count++] = number;
        }
        token = strtok_r(NULL, ", ", &next_token);
    }

    free(input_copy);
    return result;
}

void free_phydesc(struct phy_desc *phy)
{
    for (int i = 0; i < phy->num_cls; i++) {
        for (int j = 0; j < phy->regcls[i].num_reg; j++) {
            for (int k = 0; k < phy->regcls[i].regs[j].num_field; k++) {
                free(phy->regcls[i].regs[j].fields[k].field);
                free(phy->regcls[i].regs[j].fields[k].offset.numbers);
            }
            free(phy->regcls[i].regs[j].name);
        }
        free(phy->regcls[i].name);
        free(phy->regcls[i].regs);
    }
    free(phy->regcls);
    free(phy->manufactory);
    free(phy->name);
    free(phy);
}

static void put_one_field(struct phy_desc *phy, char *name, const char *value)
{
    int clsidx, regidx, fieldidx;

    assert(phy->num_cls);
    clsidx = phy->num_cls - 1;
    assert(phy->regcls[clsidx].num_reg);
    regidx = phy->regcls[clsidx].num_reg - 1;
    fieldidx = phy->regcls[clsidx].regs[regidx].num_field;
    assert(fieldidx < 16);

    phy->regcls[clsidx].regs[regidx].fields[fieldidx].field = name;
    phy->regcls[clsidx].regs[regidx].fields[fieldidx].offset = parse_input(value);
    phy->regcls[clsidx].regs[regidx].num_field ++;
}

static void put_one_reg(struct phy_desc *phy, char *name, const char *value)
{
    int clsidx, regidx;
    struct reg *reg;

    assert(phy->num_cls);

    clsidx = phy->num_cls - 1;
    regidx = phy->regcls[clsidx].num_reg;
    phy->regcls[clsidx].num_reg ++;
    phy->regcls[clsidx].regs = realloc(phy->regcls[clsidx].regs, sizeof(struct reg) * (phy->regcls[clsidx].num_reg));
    reg = &phy->regcls[clsidx].regs[regidx];
    reg->name = name;
    reg->addr = strtol(value, NULL, 16);
    reg->num_field = 0;
}

static void put_one_cls(struct phy_desc *phy, char *name)
{
    int clsidx;
    phy->regcls = realloc(phy->regcls, sizeof(struct regcls) *(phy->num_cls+1));
    phy->num_cls ++;

    clsidx = phy->num_cls - 1;
    phy->regcls[clsidx].name = name;
    phy->regcls[clsidx].num_reg = 0;
    phy->regcls[clsidx].regs = NULL;

}

static int consume_event(struct parser_state *s, yaml_event_t *event)
{
    switch (s->state) {
    case START:
        switch (event->type) {
        case YAML_MAPPING_START_EVENT:
            s->state = ACCEPT_KEY;
            break;
        case YAML_SCALAR_EVENT:
            s->state = ACCEPT_KEY;
            break;
        case YAML_SEQUENCE_START_EVENT:
            fprintf(stderr, "Unexpected sequence.\n");
            s->state = ERROR;
            break;
        case YAML_STREAM_END_EVENT:
            s->state = STOP;
            break;
        default:
            break;
        }
        break;
    case ACCEPT_LIST_CLASS:
        switch (event->type) {
        case YAML_SEQUENCE_START_EVENT:
            s->state = ACCEPT_VALUES;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting sequence: %d\n", event->type);
            s->state = ERROR;
            break;
        }
        break;
    case ACCEPT_LIST_REG:
        switch (event->type) {
        case YAML_SEQUENCE_START_EVENT:
            s->state = ACCEPT_REG_NAME;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting sequence: %d\n", event->type);
            s->state = ERROR;
            break;
        }
        break;
    case ACCEPT_REG_NAME:
        switch (event->type)
        {
        case YAML_MAPPING_START_EVENT:
            s->state = ACCEPT_KEY1;
            break;
        default:
            printf("error for reg name\n");
            s->state = ERROR;
            break;
        }
        break;
    case ACCEPT_VALUES:
        switch (event->type) {
        case YAML_MAPPING_START_EVENT:
            s->state = ACCEPT_KEY;
            break;
        case YAML_MAPPING_END_EVENT:
            s->state = START;
            break;
        case YAML_SEQUENCE_END_EVENT:
            s->state = START;
            break;
        case YAML_DOCUMENT_END_EVENT:
            s->state = START;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting mapped values: %d\n",
                    event->type);
            s->state = ERROR;
            break;
        }
        break;
    case ACCEPT_VALUES1:
        switch (event->type) {
        case YAML_MAPPING_START_EVENT:
            s->state = ACCEPT_KEY1;
            break;
        case YAML_SEQUENCE_END_EVENT:
            s->state = START;
            break;
        case YAML_DOCUMENT_END_EVENT:
            s->state = START;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting mapped values: %d\n",
                    event->type);
            s->state = ERROR;
            break;
        }
        break;
    case ACCEPT_KEY1:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->key = strdup((char*)event->data.scalar.value);
            s->state = ACCEPT_VALUE1;
            break;
        case YAML_MAPPING_END_EVENT:
            // s->accepted = 1;
            s->state = ACCEPT_VALUES1;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting key: %d\n",
                    event->type);
            s->state = ERROR;
            break;
        }

        break;
    case ACCEPT_KEY:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            if (!strcmp((char*)event->data.scalar.value, "device")) {
                s->key = strdup((char*)event->data.scalar.value);
                s->state = ACCEPT_VALUE;
            } else if (!strcmp((char*)event->data.scalar.value, "regmap")) {
                s->state = ACCEPT_LIST_REG;
            } else if (!strcmp((char*)event->data.scalar.value, "Registers")) {
                s->state = ACCEPT_LIST_CLASS;
            } else {
                s->key = strdup((char*)event->data.scalar.value);
                s->state = ACCEPT_VALUE;
            }
            break;
        case YAML_MAPPING_END_EVENT:
            // s->accepted = 1;
            s->state = ACCEPT_VALUES;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting key: %d\n",
                    event->type);
            s->state = ERROR;
            break;
        }

        break;
    case ACCEPT_VALUE:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->value = (char*)event->data.scalar.value;
            if (!strcmp(s->key, "Name")) {
                s->phy->name = strdup((char*)s->value);
            } else if (!strcmp(s->key, "Manufactory")) {
                s->phy->manufactory = strdup((char*)s->value);
            } else if (!strcasecmp(s->key, "device")) {
                s->phy->regcls[s->phy->num_cls-1].dev = strtoul((char*)s->value, NULL, 10);
            }
            free(s->key);
            s->state = ACCEPT_KEY;
            break;
        case YAML_SEQUENCE_START_EVENT:
            s->state = ACCEPT_VALUES;
            break;
        case YAML_MAPPING_START_EVENT:
            put_one_cls(s->phy, s->key);
            s->state = ACCEPT_KEY;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting value: %d\n",
                    event->type);
            s->state = ERROR;
            break;
        }
        break;
    case ACCEPT_VALUE1:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->value = (char*)event->data.scalar.value;
            put_one_reg(s->phy, s->key, s->value);
            s->state = ACCEPT_REG_FIELD_KEY;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting value: %d\n",
                    event->type);
            s->state = ERROR;
            break;
        }
        break;
    case ACCEPT_REG_FIELD_KEY:
        switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->key = strdup((char*)event->data.scalar.value);
            s->state = ACCEPT_REG_FIELD_VALUE;
            break;
        case YAML_MAPPING_END_EVENT:
            // end of reg fields, so it is end of one register, record them
            s->accepted = 1;
            s->state = ACCEPT_VALUES1;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting key: %d\n",
                    event->type);
            s->state = ERROR;
            break;
        }
        break;
    case ACCEPT_REG_FIELD_VALUE:
    switch (event->type) {
        case YAML_SCALAR_EVENT:
            s->value = (char*)event->data.scalar.value;
            s->state = ACCEPT_REG_FIELD_KEY;
            put_one_field(s->phy, s->key, s->value);
            break;
        case YAML_SEQUENCE_START_EVENT:
            s->state = ACCEPT_VALUES;
            break;
        default:
            fprintf(stderr, "Unexpected event while getting value: %d\n",
                    event->type);
            s->state = ERROR;
            break;
        }
        break;
    case ERROR:
    case STOP:
        break;
    }
    return (s->state == ERROR ? 0 : 1);
}

struct phy_desc * read_phy_yaml(const char * filename) {
    yaml_parser_t parser;
    yaml_event_t event;
    
    struct phy_desc *phy = malloc(sizeof(struct phy_desc));
    phy->num_cls = 0;
    phy->regcls = NULL;
    struct parser_state state = {.state=START, .accepted=0, .error=0, .phy = phy,};

    /* Create the Parser object. */
    yaml_parser_initialize(&parser);

    /* Set a string input. */
    FILE *fp = fopen(filename, "rb");
    yaml_parser_set_input_file(&parser, fp);

    /* Read the event sequence. */
    do {
        /* Get the next event. */
        if (!yaml_parser_parse(&parser, &event))
            goto error;

        if (!consume_event(&state, &event))
            goto error;

        if (event.type != YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
        }
    } while (event.type != YAML_STREAM_END_EVENT);
    yaml_event_delete(&event);


    /* Destroy the Parser object. */
    yaml_parser_delete(&parser);
    fclose(fp);

#if 0
    printf("\n%s, %s\n", phy->manufactory, phy->name);
    printf("regs num %d\n", phy->num);
    for (int i = 0; i < phy->num; i++) {
        printf("regs group %s, dev %d\n", phy->regs[i].name, phy->regs[i].dev);
        for (int j = 0; j < phy->regs[i].num; j++) {
            printf("%s: 0x%04x\n", phy->regs[i].map[j].name, phy->regs[i].map[j].addr);
        }
    }
#endif
    return phy;

error:

    /* Destroy the Parser object. */
    yaml_parser_delete(&parser);

    return NULL;
}